#pragma once
#include SOUND_HEADER
#include "clock.hpp"
#include <boost/variant.hpp>

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
		public:
			virtual ~ABuffer() {}
			virtual bool isStreaming() const = 0;
			// 指定の場所から任意サイズのサンプルデータを受け取る
			/*! \param[in] offset	取得したいサンプルのオフセット(bytes)
				\param[in] buffLen	受信バッファサイズ(bytes)
				\return コピーされたデータサイズ(bytes)  */
			virtual size_t getData(void* dst, uint64_t offset, size_t buffLen) const { Assert(false); }
			/*! \param[in] offset 受信オフセット(bytes) */
			virtual std::pair<const void*, size_t> getAllData(uint64_t offset) const { Assert(false); }
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
		int			_nBuffer;	//!< AudioBufferの数
		int			_readCur,	//!< GetBlockで送り出した位置
					_playedCur,	//!< 再生が終わった位置
					_writeCur;	//!< 未再生バッファの準備ができている位置
		uint64_t 	_offset;

		void _fillBuffer();
		public:
			ABufSub(HAb hAb);
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
		public:
			AWaveBatch(HRW hRW);
			bool isStreaming() const override;
			std::pair<const void*, size_t> getAllData(uint64_t offset) const override;
	};
	// Oggバッチ再生
	// ファイルからOggを読んでWaveに変換
	class AOggBatch : public ABuffer {
		ByteBuff	_buff;
		public:
			AOggBatch(HRW hRW);
			bool isStreaming() const override;
			std::pair<const void*, size_t> getAllData(uint64_t offset) const override;
	};
	// Oggストリーミング再生
	// ファイルから読み込んでストリーミング
	class AOggStream : public ABuffer {
		mutable VorbisFile	_vfile;
		mutable uint64_t 	_prevOffset;
		public:
			AOggStream(HRW hRW);
			bool isStreaming() const override;
			size_t getData(void* dst, uint64_t offset, size_t buffLen) const override;
	};

	class ASource {
		struct IState {
			virtual void play(ASource& self) {}
			virtual void pause(ASource& self) {}
			virtual void rewind(ASource& self) {}
			virtual void stop(ASource& self) {}
			virtual void update(ASource& self) { self._advanceGain(); }
			virtual void timeSeek(ASource& self, Duration tp) {
				auto st = getState();
				// 一旦止めてキューにセットし直す
				self._setState<S_Stopped>();
				self._opBuf->timeSeek(tp);
				self._refillBuffer();
				self._timePos = tp;
				self._pcmPos = CalcSampleLength(self._opBuf->getFormat(), tp);
				self._playedCur = 0;

				if(st == AState::Playing)
					self._setState<S_Playing>();
			}
			virtual void pcmSeek(ASource& self, uint64_t t) {}
			virtual void setBuffer(ASource& self, HAb hAb);
			virtual void onExit(ASource& self) {}
			virtual AState getState() const = 0;
		};
		struct S_Empty : IState {
			S_Empty(ASource& self) {
				self._pcmPos = 0;
				self._timePos = std::chrono::seconds(0);
			}
			void update(ASource& self) override {}
			AState getState() const override { return AState::Empty; }
		};
		struct S_Playing : IState {
			S_Playing(ASource& self) {
				self._dep.play();
				self._tmUpdate = Clock::now();
			}
			void onExit(ASource& self) {
				Duration dur = Clock::now() - self._tmUpdate;
				self._timePos += dur;
			}
			void pause(ASource& self) override { self._setState<S_Paused>(); }
			void rewind(ASource& self) override { self._setState<S_Initial>(); }
			void stop(ASource& self) override { self._setState<S_Stopped>(); }
			void pcmSeek(ASource& self, uint64_t t) override;
			void update(ASource& self) override;
			AState getState() const override { return AState::Playing; }
		};
		struct S_Stopped : IState {
			S_Stopped(ASource& self) { self._dep.reset(); }
			void rewind(ASource& self) override {
				self._setState<S_Initial>();
			}
			AState getState() const override { return AState::Stopped; }
		};
		struct S_Initial : IState {
			void _init(ASource& self);
			S_Initial(ASource& self) { _init(self); }
			S_Initial(ASource& self, HAb hAb) {
				self._opBuf = ABufSub(hAb);
				_init(self);
			}
			void play(ASource& self) override;
			AState getState() const override { return AState::Initial; }
		};
		struct S_Paused : IState {
			S_Paused(ASource& self) { self._dep.pause(); }
			void play(ASource& self) override {
				self._setState<S_Playing>();
			}
			void stop(ASource& self) override {
				self._setState<S_Stopped>();
			}
			AState getState() const override { return AState::Paused; }
		};

		using UPState = std::unique_ptr<IState>;
		UPState		_state;
		using OPBuf = spn::Optional<ABufSub>;
		OPBuf		_opBuf;
		ASourceDep	_dep;
		uint32_t	_nLoop;			//!< イベントキューにEv_Endがセットされてない
		Duration	_duration;		//!< nLoopを考慮した場合の曲の長さ
		Timepoint	_tmUpdate;		//!< 前回Updateを呼んだ時刻
		float		_currentGain,
					_targetGain;

		Duration	_timePos;		//!< 時間単位の再生位置
		uint64_t	_pcmPos;		//!< サンプル単位の再生位置
		uint32_t	_playedCur;		//!< 再生済みブロック数

		// フェードイン・アウト処理
		struct Fade {
			bool		bValid;
			Duration	durFadeBegin, durFadeEnd;
			float		fromGain, toGain;

			void init();
			void init(Duration cur, Duration dur, float to);
			bool apply(ASource& self, float& gain);
		} _fadeTo, _fadeOut;
		// フェード処理は大きく分けて2つ: 曲終了のフェードアウトとそれ以外
		void _applyFades();
		void _advanceGain();
		void _refillBuffer();

		// イベントキューが空でない場合は
		template <class S, class... Ts>
		void _setState(Ts&&... ts) {
			_state->onExit(*this);
			_state.reset(nullptr);
			_state.reset(new S(*this, std::forward<Ts>(ts)...));
		}
		public:
			ASource();
			ASource(const ASource&) = delete;
			ASource(ASource&& s);
			~ASource();

			void play();
			void pause();
			void rewind();
			void stop();
			//! ストリーミングキューの更新
			void update();
			//! フェードアウト時間の設定
			/*! 曲の残り時間がmsOutに満たない場合はそれに合わせて修正される
				fadeTo(0.f, dur, getEndTime() - dur); と同義
				\param[in] dur フェードアウト時間
				\param[in] bNow true=すぐにフェードアウトを始める, false=曲の末尾に合わせる */
			void setFadeOut(Duration dur, bool bNow);
			//! このまま再生を続けた場合に曲が終了する時間 (ループ, ピッチ込み)
			Timepoint getEndTime();
			/*! \param[in] gain 目標音量
				\param[in] dur 遷移時間 */
			void fadeTo(float gain, Duration dur);

			// ループ込みの長さで管理する
			AState getState() const;

			void setBuffer(HAb hAb, Duration fadeIn=std::chrono::seconds(0), uint32_t nLoop=0);
			uint32_t getLooping() const;
			void setPitch(float pitch);
			void setGain(float gain);

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
	};
	class SSrcMgr : public spn::ResMgrA<ASource, SSrcMgr> {};
	DEF_HANDLE(SSrcMgr, Ss, ASource)

	//! ASourceをひとまとめにして管理
	class AGroup {
		using SourceL = std::vector<HLSs>;
		SourceL	_source;
		int		_nActive;
		bool	_bPaused;
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

	#define mgr_sound reinterpret_cast<SoundMgr&>(SoundMgr::_ref())
	class SoundMgr : public SoundMgrDep {
		ABufMgr		_buffMgr;
		SSrcMgr 	_srcMgr;
		SGroupMgr	_sgMgr;

		public:
			using SoundMgrDep::SoundMgrDep;
			SoundMgr(const SoundMgr&) = delete;
			HLAb loadWaveBatch(HRW hRw);
			HLAb loadOggBatch(HRW hRw);
			HLAb loadOggStream(HRW hRw);

			HLSg createSourceGroup(int n);
			HLSs createSource();
			void update();
	};
}
