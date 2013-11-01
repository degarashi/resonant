#include "sound_depSL.hpp"

namespace rs {
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
		SDLAFormatCF sfmt(af);
		const SDLAFormatCF& outfmt = SoundMgr_depSL::_ref().getOutMixFormat();
		// なぜか37800Hz付近で砂嵐になるので小細工
		int srcfreq = sfmt.freq;
		if(outfmt.freq > srcfreq && srcfreq >= 37700 && srcfreq <= 37900)
			srcfreq = 37600;
		if(SDL_BuildAudioCVT(&cvt, sfmt.format, sfmt.channels, srcfreq,
								outfmt.format, outfmt.channels, outfmt.freq) != 0)
		{
			cvt.len = len;
			// 周波数変換してから書き込み
			if(cvt.len_mult > 1)
				_buff.resize(len * cvt.len_mult);
			else
				_buff.resize(cvt.len);
			cvt.buf = &_buff[0];
			std::memcpy(&_buff[0], src, len);
			SDLEC(Trap, SDL_ConvertAudio, &cvt);
			_buff.resize(cvt.len_cvt);
		} else {
			// そのまま書き込み
			_buff.resize(len);
			std::memcpy(&_buff[0], src, len);
		}
	}

	// --------------------- ASource_depSL ---------------------
	ASource_depSL::ASource_depSL() {
		auto& s = SoundMgr_depSL::_ref();

		SLDataLocator_BufferQueue loc_bufq = {
			SL_DATALOCATOR_BUFFERQUEUE, MAX_AUDIO_BLOCKNUM
		};
		const SDLAFormatCF& outFmt = SoundMgr_depSL::_ref().getOutMixFormat();
		// OutputMixと同じフォーマットで初期化
		SLDataFormat_PCM format_pcm = {
			SL_DATAFORMAT_PCM,
			static_cast<SLuint32>(outFmt.channels),
			static_cast<SLuint32>(outFmt.freq * 1000),
			SL_PCMSAMPLEFORMAT_FIXED_16,
			16,
			SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN
		};
		SLDataSource audioSrc = {&loc_bufq, &format_pcm};
		auto eItf = s.getEngineItf();
		SLDataLocator_OutputMix om = {SL_DATALOCATOR_OUTPUTMIX, s.getOutMix().refObj()};
		SLDataSink audioSink = {&om, nullptr};

		const SLInterfaceID ids[] = {SL_IID_BUFFERQUEUE, SL_IID_PLAYBACKRATE, SL_IID_PLAY, SL_IID_VOLUME};
		const SLboolean req[] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};
		SLEC_M(Trap, eItf, CreateAudioPlayer, &_aplayer.refObj(), &audioSrc, &audioSink, 4, ids, req);
		_aplayer.realize(false);

		_volItf = _aplayer.getInterface<SLVolumeItf>(SL_IID_VOLUME);
		_playItf = _aplayer.getInterface<SLPlayItf>(SL_IID_PLAY);
		_bqItf = _aplayer.getInterface<SLBufferQueueItf>(SL_IID_BUFFERQUEUE);
		_rateItf = _aplayer.getInterface<SLPlaybackRateItf>(SL_IID_PLAYBACKRATE);
		SLEC_M(Trap, _playItf, SetPlayState, SL_PLAYSTATE_STOPPED);
		SLuint32 cap;
		SLEC_M(Trap, _rateItf, GetRateRange, 0, &_rateMin, &_rateMax, &_rateStep, &cap);
	}
	ASource_depSL::ASource_depSL(ASource_depSL&& s):
		_aplayer(std::move(s._aplayer)),
		_playItf(s._playItf),
		_volItf(s._volItf),
		_bqItf(s._bqItf),
		_rateItf(s._rateItf),
		_rateMin(s._rateMin), _rateMax(s._rateMax), _rateStep(s._rateStep), _blockCount(s._blockCount)
	{}
	void ASource_depSL::play() {
		SLEC_M(Trap, _playItf, SetPlayState, SL_PLAYSTATE_PLAYING);
	}
	void ASource_depSL::reset() {
		SLEC_M(Trap, _playItf, SetPlayState, SL_PLAYSTATE_STOPPED);
		_blockCount = 0;
	}
	void ASource_depSL::pause() {
		SLEC_M(Trap, _playItf, SetPlayState, SL_PLAYSTATE_PAUSED);
	}
	void ASource_depSL::update(bool bPlaying) {
		// OpenSLでは終端まで行っても自動でStoppedにならない = なにも処理しない
	}
	bool ASource_depSL::isEnded() {
		SLBufferQueueState bqs;
		SLEC_M(Trap, _bqItf, GetState, &bqs);
		return bqs.count==0;
	}
	void ASource_depSL::setGain(float vol) {
		SLEC_M(Trap, _volItf, SetVolumeLevel, VolToMillibel(vol));
	}
	void ASource_depSL::setPitch(float pitch) {
		SLpermille p = static_cast<SLpermille>(pitch*1000);
		p = std::min(std::max(p, _rateMin), _rateMax);
		SLEC_M(Trap, _rateItf, SetRate, p);
	}
	float ASource_depSL::timeTell(float def) {
		SLmillisecond ms;
		SLEC_M(Trap, _playItf, GetPosition, &ms);
		return static_cast<float>(ms) / 1000.f;
	}
	int64_t ASource_depSL::pcmTell(int64_t def) {
		return def;
	}
	void ASource_depSL::timeSeek(float t) {}
	void ASource_depSL::pcmSeek(int64_t p) {}
	void ASource_depSL::enqueue(ABuffer_depSL& buff) {
		auto& bf = buff.getBuff();
		SLEC_M(Trap, _bqItf, Enqueue, &bf[0], bf.size());
	}
	int ASource_depSL::getUsedBlock() {
		SLBufferQueueState state;
		SLEC_M(Trap, _bqItf, GetState, &state);
		LOGI("index=%u", (uint32_t)state.playIndex);
		int ret = state.playIndex - _blockCount;
		_blockCount = state.playIndex;
		return std::max(0,ret);
	}
	void ASource_depSL::clearBlock() {
		SLEC_M0(Trap, _bqItf, Clear);
		_blockCount = 0;
	}

	// --------------------- SoundMgr_depSL ---------------------
	SoundMgr_depSL::SoundMgr_depSL(int rate): _outFormat(SDLAFormat(1,0,0,16), rate) {
		// サウンドエンジンの作成
		SLEC(Trap, slCreateEngine, &_engine.refObj(), 0, nullptr, 0, nullptr, nullptr);
		_engine.realize(false);

		// OutputMixの初期化
		_engineItf = _engine.getInterface<SLEngineItf>(SL_IID_ENGINE);
		SLEC_M(Trap, _engineItf, CreateOutputMix, &_outmix.refObj(), 0, nullptr, nullptr);
		_outmix.realize(false);
	}
	void SoundMgr_depSL::printVersions(std::ostream& os) {
		SLresult result;
		SLuint32 num;
		SLEC(Trap, slQueryNumSupportedEngineInterfaces, &num);
		os << "Enumerating OpenSL interfaces..." << std::endl;
		for(int i=0 ; i<num ; i++) {
			SLInterfaceID iid;
			SLEC(Trap, slQuerySupportedEngineInterfaces, i, &iid);
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
	SLObj& SoundMgr_depSL::getOutMix() {
		return _outmix;
	}
	const SDLAFormatCF& SoundMgr_depSL::getOutMixFormat() const {
		return _outFormat;
	}
}
