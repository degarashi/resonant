#pragma once
#include SOUND_HEADER
#include "clock.hpp"
#include <boost/variant.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/serialization/export.hpp>
#include "spinner/serialization/chrono.hpp"

namespace rs {
	Duration CalcTimeLength(int word_size, int ch, int hz, size_t buffLen);
	uint64_t CalcSampleLength(int word_size, int ch, int hz, Duration dur);
	uint64_t CalcSampleLength(const AFormatF& af, Duration dur);

	//! 共有サンプルデータソース
	class ABuffer {
		protected:
			AFormatF	_format;
			Duration	_duration;
			ABuffer();
		private:
			friend class boost::serialization::access;
			template <class Archive>
			void serialize(Archive& ar, const unsigned int ver) {
				ar & _format & _duration;
			}
		public:
			virtual ~ABuffer() {}
			virtual bool isStreaming() const = 0;
			// 指定の場所から任意サイズのサンプルデータを受け取る
			/*! \param[in] offset	取得したいサンプルのオフセット(bytes)
				\param[in] buffLen	受信バッファサイズ(bytes)
				\return コピーされたデータサイズ(bytes)  */
			virtual size_t getData(void* dst, uint64_t offset, size_t buffLen) const { Assert(Trap, false); throw 0; }
			/*! \param[in] offset 受信オフセット(bytes) */
			virtual std::pair<const void*, size_t> getAllData(uint64_t offset) const { Assert(Trap, false); throw 0; }
			Duration getDuration() const { return _duration; }
			const AFormatF& getFormat() const { return _format; }
	};
	using UPABuff = std::unique_ptr<ABuffer>;
	class ABufMgr : public spn::ResMgrA<UPABuff, ABufMgr> {};
	DEF_HANDLE(ABufMgr, Ab, UPABuff)

	//! 固有サンプルデータソース
	class ABufSub {
		HLAb		_hlAb;
		using UPBuff = std::unique_ptr<ABufferDep[]>;
		UPBuff		_abuff;
		uint32_t	_nLoop, _nLoopInit;
		int			_nBuffer;	//!< AudioBufferの数
		int			_readCur,	//!< GetBlockで送り出した位置
					_playedCur,	//!< 再生が終わった位置
					_writeCur;	//!< 未再生バッファの準備ができている位置
		uint64_t 	_offset;

		void _fillBuffer();
		public:
			ABufSub(HAb hAb, uint32_t nLoop);
			ABufSub(const ABufSub&) = delete;
			ABufSub(ABufSub&& a);

			ABufferDep* getBlock();
			void rewind();
			bool isEOF();
			//! 再生が終わったブロック番号のセット (from ASource)
			void setPlayedCursor(int cur);
			void timeSeek(Duration tp);
			void pcmSeek(uint64_t t);

			Duration getDuration() const;
			const AFormatF& getFormat() const;
	};

	// Waveバッチ再生
	// リストアは再度ファイルから読み込む
	class AWaveBatch : public ABuffer {
		ByteBuff	_buff;

		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _buff & boost::serialization::base_object<ABuffer>(*this);
		}
		AWaveBatch() = default;
		public:
			AWaveBatch(HRW hRW);
			bool isStreaming() const override;
			std::pair<const void*, size_t> getAllData(uint64_t offset) const override;
	};
	// Oggバッチ再生
	// ファイルからOggを読んでWaveに変換
	class AOggBatch : public ABuffer {
		ByteBuff	_buff;

		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _buff & boost::serialization::base_object<ABuffer>(*this);
		}
		AOggBatch() = default;
		public:
			AOggBatch(HRW hRW);
			bool isStreaming() const override;
			std::pair<const void*, size_t> getAllData(uint64_t offset) const override;
	};
	// Oggストリーミング再生
	// ファイルから読み込んでストリーミング
	class AOggStream : public ABuffer {
		VorbisFile	_vfile;
		mutable uint64_t 	_prevOffset;

		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _vfile & _prevOffset & boost::serialization::base_object<ABuffer>(*this);
		}
		AOggStream() = default;
		public:
			AOggStream(HRW hRW);
			bool isStreaming() const override;
			size_t getData(void* dst, uint64_t offset, size_t buffLen) const override;
	};
	class ASource {
		private:
			struct FadeCB {
				virtual void onFadeEnd(ASource& s) = 0;
			};
			template <class State>
			struct FadeCB_State : FadeCB {
				void onFadeEnd(ASource& s) override {
					s._setState<State>();
				}
			};
			struct IState {
				virtual void play(ASource& self, Duration fadeIn) {}
				virtual void pause(ASource& self, Duration fadeOut) {}
				virtual void stop(ASource& self, Duration fadeOut) {}
				virtual void update(ASource& self) {}
				virtual void timeSeek(ASource& self, Duration t) {}
				virtual void pcmSeek(ASource& self, int64_t t) {}
				virtual void onEnter(ASource& self, AState prev) {}
				virtual void onExit(ASource& self, AState next) {}
				virtual void setFadeTo(ASource& self, float gain, Duration dur) {}
				virtual void setBuffer(ASource& self, HAb hAb, uint32_t nLoop) {}
				virtual void sys_pause(ASource& self) {}
				virtual void sys_resume(ASource& self) {}
				virtual AState getState() const = 0;
			};

			//! 再生する音声データを持っていない
			struct S_Empty : IState {
				S_Empty(ASource& s);
				void onEnter(ASource& self, AState prev) override;
				void setBuffer(ASource& self, HAb hAb, uint32_t nLoop) override;
				AState getState() const override;
			};
			//! 音声データを持っているが、バッファにはまだ読み込まれてない
			struct S_Initial : IState {
				HLAb		_hlAb;
				uint32_t	_nLoop;

				S_Initial(ASource& s);
				S_Initial(ASource& s, HAb hAb, uint32_t nLoop);
				void _init();
				void onEnter(ASource& self, AState prev) override;
				void play(ASource& self, Duration fadeIn) override;
				void timeSeek(ASource& self, Duration t) override;
				void pcmSeek(ASource& self, int64_t t) override;
				void setBuffer(ASource& self, HAb hAb, uint32_t nLoop) override;
				AState getState() const override;
			};
			//! 再生中
			struct S_Playing : IState {
				Duration	_fadeIn;
				bool		_bSysPause;

				template <class State>
				void _fadeOut(ASource& self, Duration fadeOut);

				S_Playing(ASource& s, Duration fadeIn);
				void play(ASource& self, Duration fadeIn) override;
				void pause(ASource& self, Duration fadeOut) override;
				void stop(ASource& self, Duration fadeOut) override;
				void update(ASource& self) override;
				void timeSeek(ASource& self, Duration t) override;
				void pcmSeek(ASource& self, int64_t t) override;
				void onEnter(ASource& self, AState prev) override;
				void onExit(ASource& self, AState next) override;
				void setFadeTo(ASource& self, float gain, Duration dur) override;
				void setBuffer(ASource& self, HAb hAb, uint32_t nLoop) override;
				void sys_pause(ASource& self) override;
				void sys_resume(ASource& self) override;
				AState getState() const override;
			};
			//! 再生の一時停止。バッファもそのまま
			struct S_Paused : IState {
				S_Paused(ASource& s);
				void onEnter(ASource& self, AState prev) override;
				void play(ASource& self, Duration fadeIn) override;
				void stop(ASource& self, Duration fadeOut) override;
				void timeSeek(ASource& self, Duration t) override;
				void pcmSeek(ASource& self, int64_t t) override;
				void setBuffer(ASource& self, HAb hAb, uint32_t nLoop) override;
				AState getState() const override;
			};

			using UPState = std::unique_ptr<IState>;
			using OPBuf = spn::Optional<ABufSub>;
			HLAb		_hlAb;			//!< AudioAPIのバッファ管理
			UPState		_state,			//!< サウンド再生状態
						_nextState;
			OPBuf		_opBuf;			//!< サンプル読み出しを担う抽象クラス
			ASourceDep	_dep;			//!< 実際にAudioAPI呼び出しを行う環境依存クラス
			Duration	_duration;		//!< nLoopを考慮した場合の曲の長さ
			Timepoint	_tmUpdate;		//!< 前回Updateを呼んだ時刻
			uint32_t	_nLoop;
			float		_currentGain,
						_targetGain;
			Duration	_fadeInTime,
						_fadeOutTime;
			Duration	_timePos;		//!< 時間単位の再生位置
			uint64_t	_pcmPos;		//!< サンプル単位の再生位置
			uint32_t	_playedCur;		//!< 再生済みブロック数

			#define ASource_SEQ (_hlAb)(_state)(_nextState)(_opBuf)(_dep)\
				(_duration)(_tmUpdate)(_nLoop)(_currentGain)\
				(_targetGain)(_fadeInTime)(_fadeOutTime)(_timePos)(_pcmPos)(_playedCur)
			#define Move_CtorInner(z, obj, elem)	(elem(std::move(obj.elem)))
			#define Move_Ctor(seq, obj)				BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_FOR_EACH(Move_CtorInner, obj, seq))
			#define Move_AssignInner(z, obj, elem)	elem = std::move(obj.elem);
			#define Move_Assign(seq, obj)			BOOST_PP_SEQ_FOR_EACH(Move_AssignInner, obj, seq)

			using UPFadeCB = std::unique_ptr<FadeCB>;
			struct Fade {
				Duration	durBegin, durEnd;
				float		fromGain, toGain;
				//! フェード処理が終わった時に呼ばれるコールバック
				/*! 通常、FADE_CHANGEの物しか使われない */
				UPFadeCB	callback;

				template <class Archive>
				void serialize(Archive& ar) {
					ar & durBegin & durEnd & fromGain & toGain & callback;
				}

				Fade();
				Fade(Fade&& f);
				Fade& operator = (Fade&& f);
				void init(Duration cur, Duration dur, float from, float to, FadeCB* cb=nullptr);
				//! 現在のボリュームを計算
				float calcGain(ASource& self);
			};
			enum FadeType {
				FADE_BEGIN,		//!< 曲初めのフェードイン
				FADE_END,		//!< 曲終わりのフェードアウト
				FADE_CHANGE,	//!< 曲途中の音量変更
				NUM_FADE
			};
			Fade		_fade[NUM_FADE];

			struct Save : boost::serialization::traits<Save,
					boost::serialization::object_serializable,
					boost::serialization::track_never>
			{
				HAb			hAb;
				int			stateID;
				uint32_t	nLoop;
				float		currentGain,
							targetGain;
				Duration	fadeInTime,
							fadeOutTime,
							timePos;

				template <class Archive>
				void serialize(Archive& ar, unsigned int) {
					ar & hAb & stateID & nLoop & currentGain & targetGain &
						fadeInTime & fadeOutTime & timePos;
				}
				void pack(ASource& self);
				void unpack(ASource& self);
			};
			template <class Archive>
			void serialize(Archive& ar, const unsigned int) {
				ar & _hlAb;
				for(auto& p : _fade)
					p.serialize(ar);

				Save save;
				if(typename Archive::is_loading()) {
					ar & save;
					save.unpack(*this);
					std::cout << "load: " << _playedCur << std::endl;
				} else {
					// 保存前に再生位置を更新
					auto tm = _timePos;
					_timePos = _timePos + (Clock::now() - _tmUpdate);
					save.pack(*this);
					ar & save;
					std::cout << "save: " << _playedCur << std::endl;
					_timePos = tm;
				}

			}

			float _applyFades(float gain);
			void _advanceGain();
			void _refillBuffer(bool bClear);
			void _timeSeek(Duration t);
			void _pcmSeek(int64_t t);

			//! ステート切り替え処理
			template <class S, class... Ts>
			void _setState(Ts&&... ts) {
				AssertP(Trap, !_nextState)
				_nextState.reset(new S(*this, std::forward<Ts>(ts)...));
			}
			void _doChangeState();
			friend class boost::serialization::access;

		public:
			ASource();
			ASource(const ASource&) = delete;
			ASource(ASource&& s);
			~ASource();

			const static Duration cs_zeroDur;
			void play(Duration fadeInTime=cs_zeroDur);
			void pause(Duration fadeOutTime=cs_zeroDur);
			void stop(Duration fadeOutTime=cs_zeroDur);
			//! ストリーミングキューの更新など
			void update();
			//! 任意の音量に変更
			/*!	\param[in] gain 目標音量
				\param[in] dur 遷移時間 */
			void setFadeTo(float gain, Duration dur);
			//! 曲初めのフェードイン指定
			void setFadeIn(Duration dur);
			//! 曲終わりのフェードアウト指定
			/*! 曲の残り時間がmsOutに満たない場合はそれに合わせて修正される */
			void setFadeOut(Duration dur);

			void setBuffer(HAb hAb, uint32_t nLoop=0);
			int getLooping() const;
			int getNLoop() const;
			void setPitch(float pitch);
			void setGain(float gain);
			//! このまま再生を続けた場合に曲が終了する時間 (ループ, ピッチ込み)
			Timepoint getEndTime();

			void setRelativeMode(bool bRel);
			void setPosition(const Vec3& pos);
			void setDirection(const Vec3& dir);
			void setVelocity(const Vec3& vel);
			void setGainRange(float gmin, float gmax);
			void setAngleGain(float inner, float outer);
			void setAngleOuterGain(float gain);

			Duration timeTell();
			uint64_t pcmTell();
			void timeSeek(Duration t);
			void pcmSeek(uint64_t p);
			AState getState() const;

			void sys_pause();
			void sys_resume();
	};
	class SSrcMgr : public spn::ResMgrA<ASource, SSrcMgr> {};
	DEF_HANDLE(SSrcMgr, Ss, ASource)

	class SGroupMgr;
	//! ASourceをひとまとめにして管理
	class AGroup {
		using SourceL = std::vector<HLSs>;
		SourceL	_source;
		int		_nActive;
		bool	_bPaused;

		#define AGroup_SEQ (_source)(_nActive)(_bPaused)

		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int) {
			ar & _source & _nActive & _bPaused;
		}

		friend class spn::ResMgrA<AGroup, SGroupMgr>;
		AGroup() = default;
		public:
			AGroup(int n);
			AGroup(const AGroup& a) = delete;
			AGroup(AGroup&& a);
			void update();
			void pause();
			void resume();
			void clear();
			HSs play(HAb hAb, int nLoop=0);
			HSs fadeIn(HAb hAb, Duration fadeIn, int nLoop=0);
			HSs fadeInOut(HAb hAb, Duration fadeIn, Duration fadeOut, int nLoop=0);
			int getChannels() const;
			int getIdleChannels() const;
			int getPlayingChannels() const;
	};
	class SGroupMgr : public spn::ResMgrA<AGroup, SGroupMgr> {};
	DEF_HANDLE(SGroupMgr, Sg, AGroup)

	#define mgr_sound reinterpret_cast<::rs::SoundMgr&>(::rs::SoundMgr::_ref())
	class SoundMgr : public SoundMgrDep {
		ABufMgr		_buffMgr;
		SSrcMgr 	_srcMgr;
		SGroupMgr	_sgMgr;

		friend class boost::serialization::access;
		template <class Archive>
		void serialize(Archive& ar, const unsigned int ver) {
			makeCurrent();
			try {
				ar & _buffMgr & _srcMgr & _sgMgr;
			} catch(const std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}

		public:
			using SoundMgrDep::SoundMgrDep;
			SoundMgr(const SoundMgr&) = delete;
			HLAb loadWaveBatch(HRW hRw);
			HLAb loadOggBatch(HRW hRw);
			HLAb loadOggStream(HRW hRw);

			HLSg createSourceGroup(int n);
			HLSs createSource();
			void update();
			void resetSerializeFlag();
			void pauseAllSound();
			void resumeAllSound();
	};
}
BOOST_CLASS_EXPORT_KEY(rs::ABuffer)
BOOST_CLASS_EXPORT_KEY(rs::AWaveBatch)
BOOST_CLASS_EXPORT_KEY(rs::AOggBatch)
BOOST_CLASS_EXPORT_KEY(rs::AOggStream)
BOOST_CLASS_IMPLEMENTATION(rs::SoundMgrDep, not_serializable)
BOOST_CLASS_IMPLEMENTATION(rs::SoundMgr, object_serializable)
BOOST_CLASS_IMPLEMENTATION(rs::ABuffer, object_serializable)
BOOST_CLASS_IMPLEMENTATION(rs::AWaveBatch, object_serializable)
BOOST_CLASS_IMPLEMENTATION(rs::AOggBatch, object_serializable)
BOOST_CLASS_IMPLEMENTATION(rs::AOggStream, object_serializable)

namespace boost {
	namespace serialization {
		template <class Archive>
		inline void load_construct_data(Archive& ar, rs::SoundMgr* smgr, const unsigned int ver) {
			try {
				int rate;
				ar & rate;
				new(smgr) rs::SoundMgr(rate);
			} catch(const std::exception& e) {
				std::cout << e.what() << std::endl;
			}
		}
		template <class Archive>
		inline void save_construct_data(Archive& ar, const rs::SoundMgr* smgr, const unsigned int ver) {
			int rate = smgr->getRate();
			ar & rate;
		}
	}
}
