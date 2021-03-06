#pragma once
#include "sound_common.hpp"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define SLEC_Base(flag, act, code)		::spn::EChk_usercode##flag(AAct_##act<std::runtime_error, const char*>("OpenSLCheck"), ::rs::SLError(), SOURCEPOS, code)
#define SLEC(act, func, ...)			SLEC_Base(_a, act, func(__VA_ARGS__))
#define SLEC_M(act, obj, method, ...)	SLEC_Base(_a, act, (*obj)->method(obj, __VA_ARGS__))
#define SLEC_M0(act, obj, method)		SLEC_Base(_a, act, (*obj)->method(obj))

#define SLECA(...)						SLEC_Base(_d, __VA_ARGS__)

namespace rs {
	struct SLError {
		static const char* errorDesc(SLresult result);
		void reset() const;
		static const char* getAPIName();
	};
	std::string GetIIDString(const SLInterfaceID& iid);

	class SLObj {
		SLObjectItf	_obj;
		public:
			SLObj(SLObj&& s): _obj(s._obj) {
				s._obj = nullptr;
			}
			SLObj(const SLObj&) = delete;
			SLObj(SLObjectItf itf=nullptr): _obj(itf) {}
			virtual ~SLObj() {
				if(_obj)
					(*_obj)->Destroy(_obj);
			}
			SLObjectItf& refObj() {
				return _obj;
			}
			void realize(bool async) {
				SLEC_M(Trap, _obj, Realize, SLboolean(async));
			}
			void resume(bool async) {
				SLEC_M(Trap, _obj, Resume, async);
			}
			SLuint32 getState() {
				SLuint32 ret;
				SLEC_M(Trap, _obj, GetState, &ret);
				return ret;
			}
			template <class ITF>
			ITF getInterface(const SLInterfaceID& id) {
				ITF itf;
				SLEC_M(Trap, _obj, GetInterface, id, &itf);
				return itf;
			}
			template <class ITF>
			ITF tryGetInterface(const SLInterfaceID& id) {
				ITF itf;
				auto res = (*_obj)->GetInterface(_obj, id, &itf);
				if(res == SL_RESULT_SUCCESS)
					return itf;
				return nullptr;
			}
	};

	// OpenSLはユーザーが管理
	class ABuffer_depSL {
		constexpr static int HasBuffer = 1;
		ByteBuff	_buff;
		public:
			void writeBuffer(const AFormatF& af, const void* src, size_t len);
			const ByteBuff& getBuff() const;
	};
	using ABufferDep = ABuffer_depSL;

	class ASource_depSL {
		SLObj				_aplayer;
		SLPlayItf			_playItf;
		SLVolumeItf			_volItf;
		SLBufferQueueItf	_bqItf;
		SLPlaybackRateItf	_rateItf;
		SLpermille			_rateMin, _rateMax, _rateStep;
		SLuint32			_blockCount;
		public:
			ASource_depSL();
			ASource_depSL(ASource_depSL&& s);
			ASource_depSL(const ASource_depSL&) = delete;

			void play();
			void reset();
			void pause();
			void update(bool bPlaying);
			void setGain(float vol);
			void setPitch(float pitch);
			float timeTell(float def);
			int64_t pcmTell(int64_t def);
			void timeSeek(float t);
			void pcmSeek(int64_t p);

			void enqueue(ABuffer_depSL& buff);
			int getUsedBlock();
			void clearBlock();
			bool isEnded();

			// ---- 未対応 ----
			void setRelativeMode(bool bRel) {}
			void setPosition(const Vec3& pos) {}
			void setDirection(const Vec3& dir) {}
			void setVelocity(const Vec3& vel) {}
			void setGainRange(float gmin, float gmax) {}
			void setAngleGain(float inner, float outer) {}
			void setAngleOuterGain(float gain) {}
	};
	using ASourceDep = ASource_depSL;

	SLmillibel VolToMillibel(float vol);
	float MillibelToVol(SLmillibel mv);

	//! OpenSLのデバイス初期化等
	class SoundMgr_depSL : public spn::Singleton<SoundMgr_depSL> {
		// Androidでは何もしなくてもスレッドセーフなのでコンテキスト管理は必要ない
		SLObj			_engine;
		SLEngineItf 	_engineItf;
		SLObj			_outmix;
		SDLAFormatCF	_outFormat;

		public:
			SoundMgr_depSL(int rate);
			SoundMgr_depSL(const SoundMgr_depSL&) = delete;
			int getRate() const;

			void printVersions(std::ostream& os);
			SLEngineItf	getEngineItf() const;
			SLObj& getOutMix();
			const SDLAFormatCF& getOutMixFormat() const;

			// OpenSLはスレッドセーフなのでこれらのメソッドはダミー
			void makeCurrent() {}
			void resetCurrent() {}
			void suspend() {}
			void process() {}
	};
	using SoundMgrDep = SoundMgr_depSL;
}
