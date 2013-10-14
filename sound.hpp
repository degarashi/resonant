#pragma once
#include "sound_depAL.hpp"
#include "clock.hpp"

class ABuffer {
	protected:
		AFormatF	_format;
		ABuffer();
	public:
		virtual ~ABuffer() {}
		virtual ABufferDep* getBlock() = 0;
		virtual void rewind() = 0;
		virtual void setUsedBlock(int n) {}
		virtual void timeSeek(float t) = 0;
		virtual void pcmSeek(int64_t t) = 0;
		const AFormatF& getFormat() const { return _format; }
};

// Waveバッチ再生
// リストアは再度ファイルから読み込む
class AWaveBatch : public ABuffer {
	ABufferDep	_abuff;
	bool		_bUsed;
	public:
		AWaveBatch(sdlw::HRW hRW);
		ABufferDep* getBlock() override;
		void rewind() override;
		void timeSeek(float t) override;
		void pcmSeek(int64_t t) override;
};
// Oggバッチ再生
// ファイルからOggを読んでWaveに変換
class AOggBatch : public ABuffer {
	ABufferDep	_abuff;
	bool		_bUsed;
	public:
		AOggBatch(sdlw::HRW hRW);
		ABufferDep* getBlock() override;
		void rewind() override;
		void timeSeek(float t) override;
		void pcmSeek(int64_t t) override;
};
// Oggストリーミング再生
// ファイルから読み込んでストリーミング
class AOggStream : public ABuffer {
	VorbisFile	_vfile;
	ABufferDep	_abuff[MAX_AUDIO_BLOCKNUM];
	int			_readCur, _writeCur, _nFree;

	void _fillBuffer();

	public:
		AOggStream(sdlw::HRW hRW);
		ABufferDep* getBlock() override;
		void rewind() override;
		void setUsedBlock(int n) override;
		void timeSeek(float t) override;
		void pcmSeek(int64_t t) override;
};
using UPABuff = std::unique_ptr<ABuffer>;
class ABufMgr : public spn::ResMgrA<UPABuff, ABufMgr> {};
DEF_HANDLE(ABufMgr, Ab, UPABuff)

class ASource {
	struct IState {
		virtual void play(ASource& self) {}
		virtual void pause(ASource& self) {}
		virtual void rewind(ASource& self) {}
		virtual void stop(ASource& self) {}
		virtual void update(ASource& self) {}
		virtual void timeSeek(ASource& self, float t) {}
		virtual void pcmSeek(ASource& self, int64_t t) {}
		virtual void setBuffer(ASource& self, HAb hAb);
		virtual AState getState() const = 0;
	};
	struct S_Empty : IState {
		S_Empty(ASource& self) {
			self._pcmPos = 0;
			self._timePos = 0;
		}
		AState getState() const override { return AState::Empty; }
	};
	struct S_Playing : IState {
		S_Playing(ASource& self) { self._dep.play(); }
		void pause(ASource& self) override { self._setState<S_Paused>(); }
		void rewind(ASource& self) override { self._setState<S_Initial>(); }
		void stop(ASource& self) override { self._setState<S_Stopped>(); }
		void timeSeek(ASource& self, float t) override;
		void pcmSeek(ASource& self, int64_t t) override;
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
			self._hlAb = hAb;
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

	ASourceDep	_dep;
	using UPState = std::unique_ptr<IState>;
	UPState		_state;
	HLAb		_hlAb;
	int			_nLoop;

	Timepoint	_tmBegin;
	float		_timePos;
	int64_t		_pcmPos;

	template <class S, class... Ts>
	void _setState(Ts&&... ts) {
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
		void update();
		AState getState() const;

		void setBuffer(HAb hAb, int nLoop=0);
		int getLooping() const;
		void setPitch(float pitch);
		void setGain(float gain);

		void setRelativeMode(bool bRel);
		void setPosition(const Vec3& pos);
		void setDirection(const Vec3& dir);
		void setVelocity(const Vec3& vel);
		void setGainRange(float gmin, float gmax);
		void setAngleGain(float inner, float outer);
		void setAngleOuterGain(float gain);

		float timeTell();
		int64_t pcmTell();
		void timeSeek(float t);
		void pcmSeek(int64_t p);
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
		AGroup(AGroup&& a);
		void update();

		void pause();
		void resume();
		void clear();
		HSs play(HAb hAb, int nLoop=0);
		int getChannels() const;
		int getIdleChannels() const;
		int getPlayingChannels() const;
};
class SGroupMgr : public spn::ResMgrA<AGroup, SGroupMgr> {};
DEF_HANDLE(SGroupMgr, Sg, AGroup)

using sdlw::HRW;
#define mgr_sound reinterpret_cast<SoundMgr&>(SoundMgr::_ref())
class SoundMgr : public SoundMgrDep {
	ABufMgr		_buffMgr;
	SSrcMgr 	_srcMgr;
	SGroupMgr	_sgMgr;

	public:
		using SoundMgrDep::SoundMgrDep;
		HLAb loadWaveBatch(HRW hRw);
		HLAb loadOggBatch(HRW hRw);
		HLAb loadOggStream(HRW hRw);

		HLSg createSourceGroup(int n);
		HLSs createSource();
		void update();
};
