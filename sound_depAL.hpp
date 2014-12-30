#pragma once
#include "sound_common.hpp"
#include <AL/al.h>
#include <AL/alc.h>

#define ALEC_Base(flag, act, ...)	::spn::EChk_memory##flag<::spn::none_t>(AAct_##act<std::runtime_error>(), ::rs::ALError(), SOURCEPOS, __VA_ARGS__)
#define ALEC_Chk_Base(flag, act)	::spn::EChk##flag(AAct_##act<std::runtime_error>(), ::rs::ALError(), SOURCEPOS);
#define ALCEC_Base(flag, act, ...)	::spn::EChk_memory##flag<::spn::none_t>(AAct_##act<std::runtime_error>(), ::rs::ALCError(::rs::SoundMgr_depAL::_ref().getDevice()), SOURCEPOS, __VA_ARGS__)

#define ALEC(...)					ALEC_Base(_a, __VA_ARGS__)
#define ALCEC(...)					ALCEC_Base(_a, __VA_ARGS__)
#define OVEC(...)					OVEC_Base(_a, __VA_ARGS__)
#define ALEC_D(...)					ALEC_Base(_d, __VA_ARGS__)
#define ALCEC_D(...)				ALCEC_Base(_d, __VA_ARGS__)
#define OVEC_D(...)					OVEC_Base(_d, __VA_ARGS__)

namespace rs {
	struct ALError {
		const static std::pair<ALenum, const char*> ErrorList[];
		const char* errorDesc() const;
		void reset() const;
		const char* getAPIName() const;
	};
	struct ALCError {
		ALCdevice* _dev;
		ALCError(ALCdevice* dev);
		const static std::pair<ALCenum, const char*> ErrorList[];
		const char* errorDesc() const;
		void reset() const;
		const char* getAPIName() const;
	};
	ALenum AsALFormat(const AFormat& f);

	// OpenALはライブラリに確保したバッファに転送してから使う
	class ABuffer_depAL {
		constexpr static int HasBuffer = 0;
		// OpenALではライブラリがバッファ実体を管理するのでここにはIDだけ保存
		ALuint	_id;
		public:
			ABuffer_depAL();
			ABuffer_depAL(ABuffer_depAL&& a);
			ABuffer_depAL(const ABuffer_depAL&) = delete;
			~ABuffer_depAL();
			void writeBuffer(const AFormatF& af, const void* src, size_t len);
			ALuint getID() const;
	};
	using ABufferDep = ABuffer_depAL;

	// Bufferはあくまでデータ (batch or streaming)
	// Sourceがストリーミング制御
	class ASource_depAL {
		enum class AState {
			Initial,
			Playing,
			Paused,
			Stopped,
		};
		ALuint	_id;

		public:
			ASource_depAL();
			ASource_depAL(ASource_depAL&& a);
			ASource_depAL(const ASource_depAL&) = delete;
			~ASource_depAL();

			void play();
			void reset();
			void pause();
			void update(bool bPlaying);
			void setGain(float vol);
			void setPitch(float pitch);
			Duration timeTell(Duration def);
			int64_t pcmTell(int64_t def);
			ALuint getID() const;
			void timeSeek(float t);
			void pcmSeek(int64_t p);

			void enqueue(ABuffer_depAL& buff);
			int getUsedBlock();
			void clearBlock();

			// ---- ASourceでは未対応だけど一応実装 ----
			void setRelativeMode(bool bRel);
			void setPosition(const Vec3& pos);
			void setDirection(const Vec3& dir);
			void setVelocity(const Vec3& vel);
			void setGainRange(float gmin, float gmax);
			void setAngleGain(float inner, float outer);
			void setAngleOuterGain(float gain);
	};
	using ASourceDep = ASource_depAL;

	//! OpenALのデバイス初期化等
	class SoundMgr_depAL : public spn::Singleton<SoundMgr_depAL> {
		static SDL_atomic_t s_atmThCounter;
		static TLS<int> s_thID;

		// スレッドごとにコンテキストを用意
		// (OpenALはスレッドセーフらしいので後で変える)
		using CtxL = std::vector<ALCcontext*>;
		CtxL 		_context;
		Mutex		_mutex;
		ALCdevice*	_device;
		int			_rate;
		public:
			SoundMgr_depAL(int rate);
			SoundMgr_depAL(const SoundMgr_depAL&) = delete;
			~SoundMgr_depAL();

			void printVersions(std::ostream& os);
			ALCdevice* getDevice() const;
			int getRate() const;

			// スレッドに関連付けられたコンテキストが無ければ生成
			void makeCurrent();
			void resetCurrent();
			void suspend();
			void process();

			// ---- Listener functions ----
			static void SetPosition(const Vec3& pos);
			static void SetVelocity(const Vec3& v);
			static void SetGain(float g);
			static void SetOrientation(const Vec3& dir, const Vec3& up);
			static void SetDopplerFactor(float f);
			static void SetSpeedOfSound(float s);

			enum class DistModel : ALenum {
				InvDist = AL_INVERSE_DISTANCE,
				InvDistClamp = AL_INVERSE_DISTANCE_CLAMPED,
				LinearDist = AL_LINEAR_DISTANCE,
				LinearDistClamp = AL_LINEAR_DISTANCE_CLAMPED,
				ExponentDist = AL_EXPONENT_DISTANCE,
				ExponentDistClamp = AL_EXPONENT_DISTANCE_CLAMPED,
				Nothing = AL_NONE
			};
			static void SetDistModel(DistModel model);
	};
	using SoundMgrDep = SoundMgr_depAL;
}
