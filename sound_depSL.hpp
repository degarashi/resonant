#pragma once
#include "sound_common.hpp"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#define SLEC(func, ...)	EChk_baseA1<true, SLError>(__PRETTY_FUNCTION__, __LINE__, func(__VA_ARGS__))
#define SLEC_M(obj, method, ...)	EChk_baseA2<true, SLError>(__PRETTY_FUNCTION__, __LINE__, (*obj)->method(obj, __VA_ARGS__))
#ifdef DEBUG
	#define SLECA(...) SLEC(__VA_ARGS__)
#else
	#define SLECA(...) EChk_pass(__VA_ARGS__)
#endif
struct SLError {
	static const char* ErrorDesc(SLresult result);
	static const char* GetAPIName();
};
std::string GetIIDString(const SLInterfaceID& iid);

class SLObj {
	public:
		SLObjectItf	_obj;
	protected:
		SLObjectItf& refObj() {
			return _obj;
		}
	public:
		SLObj(const SLObj&) = delete;
		SLObj(SLObjectItf itf=nullptr): _obj(itf) {}
		virtual ~Obj() {
			if(_obj)
				(*_obj)->Destroy(_obj);
		}
		void realize(bool async) {
			SLEC_M(_obj, Realize, async);
		}
		void resume(bool async) {
			SLEC_M(_obj, Resume, async);
		}
		SLuint32 getState() {
			SLuint32 ret;
			SLEC_M(_obj, GetState, &ret);
			return ret;
		}
		template <class ITF>
		ITF getInterface(const SLInterfaceID& id) {
			IIF itf;
			SLEC_M(_obj, GetInterface, id, &itf);
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
		ASource_depSL(const ASource_depAL&) = delete;
		~ASource_depSL();

		void play();
		void reset();
		void pause();
		bool update();
		void setGain(float vol);
		void setPitch(float pitch);
		float timeTell(float def);
		int64_t pcmTell(int64_t def);
		void timeSeek(float t);
		void pcmSeek(int64_t p);

		void enqueue(ABuffer_depAL& buff);
		int getUsedBlock();
		void clearBlock();
};
using ASourceDep = ASource_depSL;

SLmillibel VolToMillibel(float vol);
float MillibelToVol(SLmillibel mv);

//! OpenSLのデバイス初期化等
class SoundMgr_depSL : public spn::Singleton<SoundMgr_depSL> {
	// Androidでは何もしなくてもスレッドセーフなのでコンテキスト管理は必要ない
	SLObj			_engine;
	SLEngineItf 	_engineItf;
	SLOutputMix		_outmix;
	SDLAFormatCF	_outFormat;

	public:
		SoundMgr_depSL(int rate);
		SoundMgr_depSL(const SoundMgr_depSL&) = delete;
		~SoundMgr_depSL();

		void printVersions(std::ostream& os);
		SLEngineItf	getEngineItf() const;
		SLOutputMix	getOutMix() const;
		const SDLAFormatCF& getOutMixFormat() const;

		// OpenSLはスレッドセーフなのでこれらのメソッドはダミー
		void makeCurrent() {}
		void resetCurrent() {}
		void suspend() {}
		void process() {}
};
using SoundMgrDep = SoundMgr_depSL;
