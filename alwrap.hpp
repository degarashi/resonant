#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include "spinner/misc.hpp"
#include "spinner/vector.hpp"
#include "sdlwrap.hpp"
#include "error.hpp"
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include <boost/concept_check.hpp>

#define ALEC(...) EChk_base<true, ALError>(__PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define ALCEC(...) EChk_base<true, ALDevice>(__PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define ALEC_Check EChk_base<true, ALError>(__PRETTY_FUNCTION__, __LINE__);
#define OVEC(...) EChk_baseA1<true, OVError>(__PRETTY_FUNCTION__, __LINE__, __VA_ARGS__);
#ifdef DEBUG
	#define ALECA(...) ALEC(__VA_ARGS__)
	#define ALCECA(...) ALCEC(__VA_ARGS__)
	#define OVECA(...) OVEC(__VA_ARGS__)
#else
	#define ALECA(...) EChk_pass(__VA_ARGS__)
	#define ALCECA(...) EChk_pass(__VA_ARGS__)
	#define OVECA(...) EChk_pass(__VA_ARGS__)
#endif

struct ALError {
	const static std::pair<ALenum, const char*> ErrorList[];
	static const char* ErrorDesc();
	static const char* GetAPIName();
};
struct OVError {
	const static std::pair<int, const char*> ErrorList[];
	static const char* ErrorDesc(int err);
	static const char* GetAPIName();
};
struct ALFormat {
	enum class Format {
		Mono8 = AL_FORMAT_MONO8,
		Mono16 = AL_FORMAT_MONO16,
		Stereo8 = AL_FORMAT_STEREO8,
		Stereo16 = AL_FORMAT_STEREO16,
		Invalid
	};

	Format	format;

	ALFormat(Format fmt=Format::Invalid);
	ALFormat(bool b16Bit, bool bStereo);
	int getBitNum() const;
	int getChannels() const;
	size_t getBlockSize() const;
	ALenum asALFormat() const;
};
struct ALFormatF : ALFormat {
	int		freq;

	ALFormatF() = default;
	ALFormatF(ALFormat fmt, int fr);
};
using spn::ByteBuff;
struct RawData {
	ALFormatF	format;
	ByteBuff	buff;

	RawData() = default;
	RawData(size_t sz);
	RawData(RawData&& d);
};

// VorbisFile wrapper
class VorbisFile {
	sdlw::HLRW		_hlRW;
	OggVorbis_File	_ovf;
	ALFormatF		_format;
	double			_dTotal;
	int64_t			_iTotal;

	public:
		static size_t ReadOGC(void* ptr, size_t blocksize, size_t nmblock, void* datasource);
		static int SeekOGC(void* datasource, ogg_int64_t offset, int whence);
		static int CloseOGC(void* datasource);
		static long TellOGC(void* datasource);
		static ov_callbacks OVCallbacksNF,
							OVCallbacks;

		VorbisFile(sdlw::HRW hRW);
		~VorbisFile();
		const ALFormatF& getFormat() const;
		//! 一括読み出し
		static RawData ReadAll(sdlw::HRW hRW);
		//! 指定サイズのデータを読み出し
		size_t read(void* dst, size_t toRead);
		bool isEOF();

		bool timeSeek(double s);
		void timeSeekPage(double s);
		bool timeSeekLap(double s);
		void timeSeekPageLap(double s);
		bool pcmSeek(int64_t pos);
		void pcmSeekPage(int64_t pos);
		bool pcmSeekLap(int64_t pos);
		void pcmSeekPageLap(int64_t pos);
		double timeLength() const;
		int64_t pcmLength() const;

		double timeTell();
		int64_t pcmTell();
};

using spn::Vec3;
#define aldevice ALDevice::_ref()
class ALCtx {
	static SDL_atomic_t s_atmThCounter;
	static sdlw::TLS<int> s_thID;

	ALCcontext*	_ctx;
	sdlw::Mutex	_mutex;
	int			_usingID;

	public:
		ALCtx();
		~ALCtx();
		void makeCurrent();
		void resetCurrent();
		void suspend();
		void process();

		// ---- Listener functions ----
		static void SetPosition(const Vec3& pos);
		static void SetVelocity(const Vec3& v);
		static void SetGain(float g);
		static void SetOrientation(const Vec3& dir, const Vec3& up);
};

class ALDevice : public spn::Singleton<ALDevice> {
	ALCdevice* _device;
	public:
		const static std::pair<ALCenum, const char*> ErrorList[];
		static const char* GetAPIName();
		static const char* ErrorDesc();

		ALDevice();
		~ALDevice();
		void printVersions(std::ostream& os);
		ALCdevice* getDevice() const;

		static void SetDopplerFactor(float f);
		static void SetSpeedOfSound(float s);

		enum class DistModel : ALenum {
			InvDist = AL_INVERSE_DISTANCE,
			InvDistClamp = AL_INVERSE_DISTANCE_CLAMPED,
			LinearDist = AL_LINEAR_DISTANCE,
			LinearDistClamp = AL_LINEAR_DISTANCE_CLAMPED,
			ExponentDist = AL_EXPONENT_DISTANCE,
			ExponentDistClamp = AL_EXPONENT_DISTANCE_CLAMPED,
			None = AL_NONE
		};
		static void SetDistModel(DistModel model);
};

enum class AState {
	Initial,
	Playing,
	Paused,
	Stopped,
	Empty
};
class ALSource;
class ALBuffer {
	ALuint		_id;
	ALFormat	_format;
	protected:
		ALBuffer();
	public:
		virtual ~ALBuffer();
		void writeBuffer(const ALFormatF& af, const void* src, size_t len);
		ALuint getID() const;

		virtual void atState(ALuint srcId, AState state) = 0;
		virtual void update(ALuint srcId) = 0;
};

class ALStaticBuffer : public ALBuffer {
	protected:
		ALStaticBuffer() = default;
	public:
		void atState(ALuint srcId, AState state) override;
		void update(ALuint srcId) override;
};
// Waveバッチ再生
class ALWaveBatch : public ALStaticBuffer {
	public:
		ALWaveBatch(sdlw::HRW hRW);
};
extern ov_callbacks OVCallbacksNF, OVCallbacks;
// Oggバッチ再生
class ALOggBatch : public ALStaticBuffer {
	public:
		ALOggBatch(sdlw::HRW hRW);
};
// Oggストリーミング再生
class ALOggStream : public ALBuffer {
	constexpr static int BLOCKNUM = 3,
						BLOCKSIZE = 4096;
	VorbisFile	_vfile;
	ALuint		_abuff[BLOCKNUM];
	int			_readCur, _writeCur;

	void _fillChunk(ALuint id, bool bForce);
	bool _checkUsedBlock(ALuint id);
	void _clearBlock(ALuint id);

	public:
		ALOggStream(sdlw::HRW hRW);
		~ALOggStream();
		void atState(ALuint srcId, AState state) override;
		void update(ALuint srcId) override;
};
#define mgr_abuff ALBufMgr::_ref()
using UPABuff = std::unique_ptr<ALBuffer>;
class ALBufMgr : public spn::ResMgrA<UPABuff, ALBufMgr> {};
DEF_HANDLE(ALBufMgr, Ab, UPABuff)

class ALSource {
	struct IState {
		ALSource& self;
		IState(ALSource& s): self(s) {}

		virtual void play();
		virtual void pause();
		virtual void rewind();
		virtual void stop();
		virtual void update() {}
		virtual void setBuffer(HAb hAb);
		virtual AState getState() const = 0;
	};
	struct S_Empty : IState {
		using IState::IState;
		void play() override {}
		void pause() override {}
		void rewind() override {}
		void stop() override {}
		AState getState() const override { return AState::Empty; }
	};
	struct S_Playing : IState {
		using IState::IState;
		void play() override {}
		void update() override {
			self._hlAb.ref()->update(self.getID()); }
		AState getState() const override { return AState::Playing; }
	};
	struct S_Stopped : IState {
		using IState::IState;
		void pause() override {}
		void stop() override {}
		void play() override;
		AState getState() const override { return AState::Stopped; }
	};
	struct S_Initial : IState {
		S_Initial(ALSource& src): IState(src) {}
		S_Initial(ALSource& src, HAb hAb): IState(src) {
			self._hlAb = hAb; }
		void pause() override {}
		void rewind() override {}
		void stop() override {}
		AState getState() const override { return AState::Initial; }
	};
	struct S_Paused : IState {
		using IState::IState;
		void pause() override {}
		AState getState() const override { return AState::Paused; }
	};

	ALuint		_id;
	using UPState = std::unique_ptr<IState>;
	UPState		_state;
	HLAb		_hlAb;

	template <class S, class... Ts>
	void _setState(Ts&&... ts) {
		_state.reset(new S(*this, std::forward<Ts>(ts)...));
		if(_hlAb.valid())
			_hlAb.ref()->atState(getID(), _state->getState());
	}
	public:
		ALSource();
		~ALSource();

		void play() const;
		void pause() const;
		void rewind() const;
		void stop() const;
		void update() const;
		AState getState() const;

		void setBuffer(HAb hAb);
		void setLooping(bool bLoop) const;
		bool isLooping() const;
		void setRelativeMode(bool bRel);
		void setPitch(float pitch);
		void setPosition(const Vec3& pos);
		void setDirection(const Vec3& dir);
		void setVelocity(const Vec3& vel);
		void setGain(float gain);
		void setGainRange(float gmin, float gmax);
		void setAngleGain(float inner, float outer);
		void setAngleOuterGain(float gain);

		float timeTell() const;
		int64_t pcmTell() const;
		void timeSeek(float t) const;
		void pcmSeek(int64_t p) const;
		ALuint getID() const;
};

//! ALSourceをひとまとめにして管理
class ALGroup {
	using SourceL = std::vector<ALSource>;
	SourceL	_source;
	public:
};
#define mgr_sound SoundMgr::_ref()
class SoundMgr : public spn::ResMgrA<ALGroup, SoundMgr> {};

