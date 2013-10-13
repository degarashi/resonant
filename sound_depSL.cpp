#include "sound_depSL.hpp"

// --------------------- SLError ---------------------
namespace {
	#define DEF_ERROR_PAIR(name) {name, #name},
	const std::pair<SLresult, const char*> c_slerror[] = {
		DEF_ERROR_PAIR(SL_RESULT_SUCCESS)
		DEF_ERROR_PAIR(SL_RESULT_PRECONDITIONS_VIOLATED)
		DEF_ERROR_PAIR(SL_RESULT_PARAMETER_INVALID)
		DEF_ERROR_PAIR(SL_RESULT_MEMORY_FAILURE)
		DEF_ERROR_PAIR(SL_RESULT_RESOURCE_ERROR)
		DEF_ERROR_PAIR(SL_RESULT_RESOURCE_LOST)
		DEF_ERROR_PAIR(SL_RESULT_IO_ERROR)
		DEF_ERROR_PAIR(SL_RESULT_BUFFER_INSUFFICIENT)
		DEF_ERROR_PAIR(SL_RESULT_CONTENT_CORRUPTED)
		DEF_ERROR_PAIR(SL_RESULT_CONTENT_UNSUPPORTED)
		DEF_ERROR_PAIR(SL_RESULT_CONTENT_NOT_FOUND)
		DEF_ERROR_PAIR(SL_RESULT_PERMISSION_DENIED)
		DEF_ERROR_PAIR(SL_RESULT_FEATURE_UNSUPPORTED)
		DEF_ERROR_PAIR(SL_RESULT_INTERNAL_ERROR)
		DEF_ERROR_PAIR(SL_RESULT_UNKNOWN_ERROR)
		DEF_ERROR_PAIR(SL_RESULT_OPERATION_ABORTED)
		DEF_ERROR_PAIR(SL_RESULT_CONTROL_LOST)
	};
}
const char* SLError::GetAPIName() {
	return "OpenSL";
}
const char* SLError::ErrorDesc(SLresult result) {
	if(result != SL_RESULT_SUCCESS) {
		for(auto& p : c_slerror) {
			if(p.first == result)
				return p.second;
		}
		return "unknown error";
	}
	return nullptr;
}


namespace {
	#define DEF_IIF_PAIR(name) {name, #name},
	const std::pair<const SLInterfaceID&, const std::string> c_pair[] = {
		DEF_IIF_PAIR(SL_IID_NULL)
		DEF_IIF_PAIR(SL_IID_OBJECT)
		DEF_IIF_PAIR(SL_IID_AUDIOIODEVICECAPABILITIES)
		DEF_IIF_PAIR(SL_IID_LED)
		DEF_IIF_PAIR(SL_IID_VIBRA)
		DEF_IIF_PAIR(SL_IID_METADATAEXTRACTION)
		DEF_IIF_PAIR(SL_IID_METADATATRAVERSAL)
		DEF_IIF_PAIR(SL_IID_DYNAMICSOURCE)
		DEF_IIF_PAIR(SL_IID_OUTPUTMIX)
		DEF_IIF_PAIR(SL_IID_PLAY)
		DEF_IIF_PAIR(SL_IID_PREFETCHSTATUS)
		DEF_IIF_PAIR(SL_IID_PLAYBACKRATE)
		DEF_IIF_PAIR(SL_IID_SEEK)
		DEF_IIF_PAIR(SL_IID_RECORD)
		DEF_IIF_PAIR(SL_IID_EQUALIZER)
		DEF_IIF_PAIR(SL_IID_VOLUME)
		DEF_IIF_PAIR(SL_IID_DEVICEVOLUME)
		DEF_IIF_PAIR(SL_IID_BUFFERQUEUE)
		DEF_IIF_PAIR(SL_IID_PRESETREVERB)
		DEF_IIF_PAIR(SL_IID_ENVIRONMENTALREVERB)
		DEF_IIF_PAIR(SL_IID_EFFECTSEND)
		DEF_IIF_PAIR(SL_IID_3DGROUPING)
		DEF_IIF_PAIR(SL_IID_3DCOMMIT)
		DEF_IIF_PAIR(SL_IID_3DLOCATION)
		DEF_IIF_PAIR(SL_IID_3DDOPPLER)
		DEF_IIF_PAIR(SL_IID_3DSOURCE)
		DEF_IIF_PAIR(SL_IID_3DMACROSCOPIC)
		DEF_IIF_PAIR(SL_IID_MUTESOLO)
		DEF_IIF_PAIR(SL_IID_DYNAMICINTERFACEMANAGEMENT)
		DEF_IIF_PAIR(SL_IID_MIDIMESSAGE)
		DEF_IIF_PAIR(SL_IID_MIDIMUTESOLO)
		DEF_IIF_PAIR(SL_IID_MIDITEMPO)
		DEF_IIF_PAIR(SL_IID_MIDITIME)
		DEF_IIF_PAIR(SL_IID_AUDIODECODERCAPABILITIES)
		DEF_IIF_PAIR(SL_IID_AUDIOENCODERCAPABILITIES)
		DEF_IIF_PAIR(SL_IID_AUDIOENCODER)
		DEF_IIF_PAIR(SL_IID_BASSBOOST)
		DEF_IIF_PAIR(SL_IID_PITCH)
		DEF_IIF_PAIR(SL_IID_RATEPITCH)
		DEF_IIF_PAIR(SL_IID_VIRTUALIZER)
		DEF_IIF_PAIR(SL_IID_VISUALIZATION)
		DEF_IIF_PAIR(SL_IID_ENGINE)
		DEF_IIF_PAIR(SL_IID_ENGINECAPABILITIES)
		DEF_IIF_PAIR(SL_IID_THREADSYNC)
		DEF_IIF_PAIR(SL_IID_ANDROIDEFFECT)
		DEF_IIF_PAIR(SL_IID_ANDROIDEFFECTSEND)
		DEF_IIF_PAIR(SL_IID_ANDROIDEFFECTCAPABILITIES)
		DEF_IIF_PAIR(SL_IID_ANDROIDCONFIGURATION)
		DEF_IIF_PAIR(SL_IID_ANDROIDSIMPLEBUFFERQUEUE)
	};
}
std::string GetIIDString(const SLInterfaceID& iid) {
	for(auto& p : c_pair) {
		if(p.first == iid)
			return p.second;
	}
	std::stringstream ss;
	ss << "unknown: " << std::hex << iid->time_low;
	return ss.str();
}
SLmillibel VolToMillibel(float vol) {
	auto volm = static_cast<int32_t>(std::log(vol) * 2000.0f);
	if(volm < SL_MILLIBEL_MIN)
		return SL_MILLIBEL_MIN;
	return volm;
}
float MillibelToVol(SLmillibel mv) {
	float mvf = mv / 2000.0f;
	return std::pow(10.0f, mvf);
}

// --------------------- ABuffer_depSL ---------------------
const ByteBuff& ABuffer_depSL::getBuff() const {
	return _buff;
}
void ABuffer_depSL::writeBuffer(const AFormatF& af, const void* src, size_t len) {
	SDL_AudioCVT cvt;
	SDLAFormatCF sfmt(af, 2);
	const SDLAFormatCF& outfmt = SoundMgr_depSL::_ref().getOutMixFormat();
	if(SDL_buildAudioCVT(&cvt, sfmt.format, sfmt.channels, sfmt.freq,
							outfmt.format, outfmt.channels, outfmt.freq) != 0)
	{
		// 周波数変換してから書き込み
		cvt.buf = reinterpret_cast<const Uint8*>(src);
		cvt.len = len;
		if(cvt.len_ratio > 1.0)
			_buff.resize(0);
		SDLEC(SDL_ConvertAudio, &cvt);
		std::memcpy(&_buff[0], cvt.buf, cvt.len_cvt);
	} else {
		// そのまま書き込み
		_buff.resize(len);
		std::memcpy(&_buff[0], src, len);
	}
}

// --------------------- ASource_depSL ---------------------
ASource_depSL::ASource_depSL() {
	auto& s = SoundMgr_depSL::_ref();

	SLDataLocator_SimpleBufferQueue loc_bufq = {
		SL_DATALOCATOR_SIMPLEBUFFERQUEUE, MAX_AUDIO_BLOCKNUM
	};
	const SDLAFormatCF& outFmt = SoundMgr_depSL::_ref().getOutMixFormat();
	// とりあえず16bit, 2chで初期化
	SLDataFormat_PCM format_pcm = {
		SL_DATAFORMAT_PCM,
		outFmt.channels,
		outFmt.freq * 1000,
		SL_PCMSAMPLEFORMAT_FIXED_16,
		16,
		SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN
	};
	SLDataSource audioSrc = {&loc_bufq, &format_pcm};
	auto& s = SoundMgr_depSL::_ref();
	auto eItf = s.getEngineItf();
	SLDataLocator_OutputMix om = {SL_DATALOCATOR_OUTPUTMIX, s.getOutMix()};
	SLDataSink audioSink = {&om, nullptr};

	const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_PLAYBACKRATE, SL_IID_PLAY, SL_IID_VOLUME};
	const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
	SLEC_M(eItf, CreateAudioPlayer, &_aplayer.refObj(), &audioSrc, &audioSink, 4, ids, req);
	_aplayer.realize(false);

	_volItf = _aplayer.getInterface<SLVolumeItf>(SL_IID_VOLUME);
	_playItf = _aplayer.getInterface<SLPlayItf>(SL_IID_PLAY);
	_bqItf = _aplayer.getInterface<SLBufferQueue>(SL_IID_BUFFERQUEUE);
	_rateItf = _aplayer.getInterface<SLPlaybackRateItf>(SL_IID_PLAYBACKRATE);
	SLEC_M(_playItf, SetPlayState, SL_PLAYSTATE_STOPPED);
	SLuint32 cap;
	SLEC_M(_rateItf, GetRateRange, &_rateMin, &_rateMax, &_rateStep, &cap);
}
void ASource_depSL::play() {
	SLEC_M(_playItf, SetPlayState, SL_PLAYSTATE_PLAYING);
}
void ASource_depSL::reset() {
	SLEC_M(_playItf, SetPlayState, SL_PLAYSTATE_STOPPED);
}
void ASource_depSL::pause() {
	SLEC_M(_playItf, SetPlayState, SL_PLAYSTATE_PAUSED);
}
bool ASource_depSL::update() {
}
void ASource_depSL::setGain(float vol) {
	SLEC_M(_volItf, SetVolumeLevel, VolToMillibel(vol));
}
void ASource_depSL::setPitch(float pitch) {
	SLpermille p = static_cast<SLpermille>(pitch*1000);
	p = std::min(std::max(p, _ptcMin), _ptcMax);
	SLEC_M(_rateItf, SetRate, p);
}
float ASource_depSL::timeTell(float def) {
	SLmillisecond ms;
	SLEC_M(_playItf, GetPosition, &ms);
	return static_cast<float>(ms) / 1000.f;
}
int64_t ASource_depSL::pcmTell(int64_t def) {
	return def;
}
void ASource_depSL::timeSeek(float t) {}
void ASource_depSL::pcmSeek(int64_t p) {}
void ASource_depSL::enqueue(ABuffer_depSL& buff) {
	auto& bf = buff.getBuff();
	SLEC_M(_bqItf, Enqueue, &bf[0], bf.size());
}
int ASource_depSL::getUsedBlock() {
	SLBufferQueueState state;
	SLEC_M(_bqItf, GetState, &state);
	int ret = state.count - _blockCount;
	_blockCount = state.count;
	return ret;
}
void ASource_depSL::clearBlock() {
	SLEC_M(_bqItf, Clear);
}

// --------------------- SoundMgr_depSL ---------------------
SoundMgr_depSL::SoundMgr_depSL(int rate): _outFormat(SDLAFormat(1,0,0,16), 2, rate) {
	// サウンドエンジンの作成
	SLEC(slCreateEngine, &_engine.refObj(), 0, nullptr, 0, nullptr, nullptr);
	_engine.realize(false);

	// OutputMixの初期化
	SLEC_M(_engine, GetInterface, SL_IID_ENGINE, &_engineItf);
	SLEC_M(_engineItf, CreateOutputMix, &_outmix, 0, nullptr, nullptr);
	_outmix = _engine.getInterface<SLOutputMix>(SL_IID_OUTPUTMIX);
	_outmix.realize(false);
}
void SoundMgr_depSL::printVersions(std::ostream& os) {
	SLresult result;
	SLuint32 num;
	SLEC(slQueryNumSupportedEngineInterfaces, &num);
	os << "Enumerating OpenSL interfaces..." << std::endl;
	for(int i=0 ; i<num ; i++) {
		SLInterfaceID iid;
		SLEC(slQuerySupportedEngineInterfaces, i, &iid);
		os << GetIIDString(iid) << std::endl;
	}

	auto capItf = _engine.tryGetInterface<SLEngineCapabilitiesItf>(SL_IID_ENGINECAPABILITIES);
	if(capItf) {
		SLuint16 num;
		(*capItf)->QuerySupportedProfiles(capItf, &num);
		os << "Profiles: ";
		if(num & SL_PROFILES_PHONE)
			os << "[Phone] ";
		if(num & SL_PROFILES_MUSIC)
			os << "[Music] ";
		if(num & SL_PROFILES_GAME)
			os << "[Game] ";
		os << std::endl;
	}
}
SLEngineItf SoundMgr_depSL::getEngineItf() const {
	return _engineItf;
}
SLOutputMix SoundMgr_depSL::getOutMix() const {
	return _outmix;
}
const SDLAFormatCF& SoundMgr_depSL::getOutMixFormat() const {
	return _outFormat;
}
