#include "alwrap.hpp"

// --------------------- ALFormat ---------------------
ALFormat::ALFormat(Format fmt): format(fmt) {}
ALFormat::ALFormat(bool b16Bit, bool bStereo) {
	if(b16Bit)
		format = bStereo ? Format::Stereo16 : Format::Mono16;
	else
		format = bStereo ? Format::Stereo8 : Format::Mono8;
}

int ALFormat::getBitNum() const {
	if(format==Format::Mono8 || format==Format::Stereo8)
		return 8;
	return 16;
}
int ALFormat::getChannels() const {
	if(format==Format::Mono8 || format==Format::Mono16)
		return 1;
	return 2;
}
ALenum ALFormat::asALFormat() const {
	return static_cast<ALenum>(format);
}
size_t ALFormat::getBlockSize() const {
	return getBitNum()/8 * getChannels();
}
// --------------------- ALFormatF ---------------------
ALFormatF::ALFormatF(ALFormat fmt, int fr): ALFormat(fmt), freq(fr) {}

// --------------------- ALError ---------------------
const std::pair<ALenum, const char*> ALError::ErrorList[] = {
	{AL_NO_ERROR, "No error"},
	{AL_INVALID_NAME, "Invalid name paramater passed to AL call"},
	{AL_INVALID_ENUM, "Invalid enum parameter passed to AL call"},
	{AL_INVALID_VALUE, "Invalid value parameter passed to AL call"},
	{AL_INVALID_OPERATION, "Illegal AL call"},
	{AL_OUT_OF_MEMORY, "Not enough memory"}
};
const char* ALError::GetAPIName() {
	return "OpenAL";
}
const char* ALError::ErrorDesc() {
	ALenum num = alGetError();
	if(num != AL_NO_ERROR) {
		for(auto& p : ErrorList) {
			if(p.first == num)
				return p.second;
		}
		return "unknown error";
	}
	return nullptr;
}

// --------------------- ALDevice ---------------------
ALDevice::ALDevice() {
	_device = alcOpenDevice(nullptr);
	if(!_device)
		throw std::runtime_error("");
}
ALDevice::~ALDevice() {
	alcCloseDevice(_device);
}
ALCdevice* ALDevice::getDevice() const { return _device; }
void ALDevice::printVersions(std::ostream& os) {
	os << "version: " << alGetString(AL_VERSION) << std::endl
	<< "vendor: " << alGetString(AL_VENDOR) << std::endl
	<< "renderer: " << alGetString(AL_RENDERER) << std::endl
	<< "extensions: " << alGetString(AL_EXTENSIONS) << std::endl;
}
const std::pair<ALCenum, const char*> ALDevice::ErrorList[] = {
	{ALC_NO_ERROR, "No error"},
	{ALC_INVALID_DEVICE, "Invalid device handle"},
	{ALC_INVALID_CONTEXT, "Invalid context handle"},
	{ALC_INVALID_ENUM, "Invalid enum parameter passed to an ALC call"},
	{ALC_INVALID_VALUE, "Invalid value parameter passed to an ALC call"},
	{ALC_OUT_OF_MEMORY, "Out of memory"}
};
const char* ALDevice::GetAPIName() {
	return "OpenAL_C";
}
const char* ALDevice::ErrorDesc() {
	ALCenum num = alcGetError(ALDevice::_ref().getDevice());
	if(num != ALC_NO_ERROR) {
		for(auto& p : ErrorList) {
			if(p.first == num)
				return p.second;
		}
		return "unknown error";
	}
	return nullptr;
}
void ALDevice::SetDopplerFactor(float f) {
// 	alGetFloat(AL_DOPPLER_FACTOR);
	ALECA(alDopplerFactor, f);
}
void ALDevice::SetSpeedOfSound(float s) {
// 	alGetFloat(AL_SPEED_OF_SOUND);
	ALECA(alSpeedOfSound, s);
}
void ALDevice::SetDistModel(DistModel model) {
// 	alGetInteger(AL_DISTANCE_MODEL);
	ALECA(alDistanceModel, static_cast<ALenum>(model));
}

// --------------------- ALCtx ---------------------
SDL_atomic_t ALCtx::s_atmThCounter = {};
sdlw::TLS<int> ALCtx::s_thID = []() {
	return SDL_AtomicAdd(&s_atmThCounter, 0x01)+1;
}();

ALCtx::ALCtx(): _usingID(0) {
	_ctx = ALCEC(alcCreateContext, aldevice.getDevice(), nullptr);
	makeCurrent();
}
ALCtx::~ALCtx() {
	resetCurrent();
	ALCEC(alcDestroyContext, _ctx);
}
void ALCtx::makeCurrent() {
	sdlw::UniLock lk(_mutex);
	Assert(_usingID == 0)
	_usingID = s_thID.get();
	ALCECA(alcMakeContextCurrent, _ctx);
}
void ALCtx::resetCurrent() {
	sdlw::UniLock lk(_mutex);
	Assert(_usingID == s_thID.get() ||
			_usingID == 0)
	_usingID = 0;
	ALCECA(alcMakeContextCurrent, nullptr);
}
void ALCtx::suspend() {
	ALCECA(alcSuspendContext, _ctx);
}
void ALCtx::process() {
	ALCECA(alcProcessContext, _ctx);
}

void ALCtx::SetPosition(const Vec3& pos) {
	ALECA(alListener3f, AL_POSITION, pos.x, pos.y, pos.z);
}
void ALCtx::SetVelocity(const Vec3& v) {
	ALECA(alListener3f, AL_VELOCITY, v.x, v.y, v.z);
}
void ALCtx::SetGain(float g) {
	ALECA(alListenerf, AL_GAIN, g);
}
void ALCtx::SetOrientation(const Vec3& dir, const Vec3& up) {
	const float tmp[] = {dir.x, dir.y, dir.z,
						up.x, up.y, up.z};
	ALECA(alListenerfv, AL_ORIENTATION, tmp);
}

// --------------------- ALBuffer ---------------------
ALBuffer::ALBuffer() {
	ALEC(alGenBuffers, 1, &_id);
}
ALBuffer::~ALBuffer() {
	ALEC(alDeleteBuffers, 1, &_id);
}
ALuint ALBuffer::getID() const {
	return _id;
}
void ALBuffer::writeBuffer(const ALFormatF& af, const void* src, size_t len) {
	_format = af;
	ALECA(alBufferData, _id, af.asALFormat(), src, len, af.freq);
}

// --------------------- ALStaticBuffer ---------------------
void ALStaticBuffer::atState(ALuint srcId, AState state) {
	if(state == AState::Initial)
		ALECA(alSourcei, srcId, AL_BUFFER, getID());
}
void ALStaticBuffer::update(ALuint srcId) {}

// --------------------- ALWaveBatch ---------------------
ALWaveBatch::ALWaveBatch(sdlw::HRW hRW) {
	auto& rw = hRW.ref();

	struct SDLMem {
		Uint8* ptr = nullptr;
		~SDLMem() {
			if(ptr)
				SDL_free(ptr);
		}
	};
	SDL_AudioSpec spec;
	SDLMem buff;
	Uint32 buffLen;
	SDLEC(SDL_LoadWAV_RW, hRW.ref().getOps(), 0, &spec, &buff.ptr, &buffLen);
	ALFormat fmt(SDL_AUDIO_BITSIZE(spec.format) > 8, spec.channels!=1);
	ALFormatF af(fmt, spec.freq);
	writeBuffer(af, buff.ptr, buffLen);
}

const char* OVError::GetAPIName() {
	return "OggVorbis";
}
const std::pair<int, const char*> OVError::ErrorList[] = {
	{OV_HOLE, "Vorbisfile encoutered missing or corrupt data in the bitstream"},
	{OV_EREAD, "A read from media returned an error"},
	{OV_EFAULT, "Internal logic fault; indicates a bug or heap/stack corruption"},
	{OV_EIMPL, "Feature not implemented"},
	{OV_EINVAL, "Either an invalid argument, or incompletely initialized argument passed to a call"},
	{OV_ENOTVORBIS, "Bitstream does not contain any Vorbis data"},
	{OV_EBADHEADER, "Invalid Vorbis bitstream header"},
	{OV_EVERSION, "Vorbis version mismatch"},
	{OV_EBADLINK, "The given link exists in the Vorbis data stream, but is not decipherable due to garbacge or corruption"},
	{OV_ENOSEEK, "The given stream is not seekable"}
};
const char* OVError::ErrorDesc(int err) {
	if(err < 0) {
		for(auto& p : ErrorList) {
			if(p.first == err)
				return p.second;
		}
		return "unknown error";
	}
	return nullptr;
}
RawData::RawData(size_t sz): buff(sz) {}
RawData::RawData(RawData&& rd): format(rd.format), buff(std::move(rd.buff)) {}
// --------------------- ALOggBatch ---------------------
ALOggBatch::ALOggBatch(sdlw::HRW hRW) {
	RawData rd = VorbisFile::ReadAll(hRW);
	writeBuffer(rd.format, &rd.buff[0], rd.buff.size());
}

// --------------------- ALOggStream ---------------------
ALOggStream::ALOggStream(sdlw::HRW hRW): _vfile(hRW), _readCur(0), _writeCur(0) {
	ALEC(alGenBuffers, int(BLOCKNUM), _abuff);
}
ALOggStream::~ALOggStream() {
	ALEC(alDeleteBuffers, int(BLOCKNUM), _abuff);
}
void ALOggStream::_fillChunk(ALuint id, bool bForce) {
	const auto& af = _vfile.getFormat();
	ALenum fmt = af.asALFormat();
	while(!_vfile.isEOF() && (_writeCur != _readCur || bForce)) {
		uint8_t buff[BLOCKSIZE];
		size_t nRead = _vfile.read(buff, BLOCKSIZE);
		ALECA(alBufferData, _abuff[_writeCur], fmt, buff, nRead, af.freq);
		ALECA(alSourceQueueBuffers, id, 1, _abuff + _writeCur);

		_writeCur = (_writeCur + 1) % BLOCKNUM;
		bForce = false;
	}
}
bool ALOggStream::_checkUsedBlock(ALuint id) {
	int nproc;
	ALECA(alGetSourcei, id, AL_BUFFERS_PROCESSED, &nproc);
	if(nproc > 0) {
		ALuint ids[BLOCKNUM];
		ALECA(alSourceUnqueueBuffers, id, nproc, ids);
		_readCur = (_readCur + nproc) % BLOCKNUM;
		return nproc == BLOCKNUM;
	}
	int st;
	alGetSourcei(id, AL_SOURCE_STATE, &st);
	if(st == AL_STOPPED) {
		int asd = 0;
	}
	return false;
}
void ALOggStream::_clearBlock(ALuint id) {
	ALECA(alSourcei, id, AL_BUFFER, 0);
}
void ALOggStream::atState(ALuint srcId, AState state) {
	if(state == AState::Initial) {
		_clearBlock(srcId);
		_vfile.pcmSeekPage(0);
		_readCur = _writeCur = 0;
		// 最初のブロックを読み込んでキューに入れる
		_fillChunk(srcId, true);
	} else if(state == AState::Stopped)
		_clearBlock(srcId);
}
void ALOggStream::update(ALuint srcId) {
	bool bForce = _checkUsedBlock(srcId);
	_fillChunk(srcId, bForce);
}

// --------------------- ALSource ---------------------
void ALSource::IState::play(ALSource& self) {
	ALECA(alSourcePlay, self.getID());
	self._setState<S_Playing>();
}
void ALSource::IState::pause(ALSource& self) {
	ALECA(alSourcePause, self.getID());
	self._setState<S_Paused>();
}
void ALSource::IState::rewind(ALSource& self) {
	ALECA(alSourceRewind, self.getID());
	self._setState<S_Initial>();
}
void ALSource::IState::stop(ALSource& self) {
	ALECA(alSourceStop, self.getID());
	self._setState<S_Stopped>();
}
void ALSource::IState::setBuffer(ALSource& self, HAb hAb) {
	stop(self);
	self._setState<S_Initial>(self, hAb);
}
void ALSource::S_Stopped::play(ALSource& self) {
	auto& self2 = self;
	self2._setState<S_Initial>();
	self2.play();
}

ALSource::ALSource() {
	ALEC(alGenSources, 1, &_id);
	_setState<S_Empty>();
}
ALSource::ALSource(ALSource&& s): _id(s._id), _state(std::move(s._state)), _hlAb(std::move(s._hlAb)) {}
void ALSource::setRelativeMode(bool bRel) {
	ALECA(alSourcei, _id, AL_SOURCE_RELATIVE, bRel ? AL_TRUE : AL_FALSE);
}
ALSource::~ALSource() {
	if(_state) {
		stop();
		ALECA(alDeleteSources, 1, &_id);
	}
}
void ALSource::play() {
	_state->play(*this);
}
void ALSource::pause() {
	_state->pause(*this);
}
void ALSource::rewind() {
	_state->rewind(*this);
}
void ALSource::stop() {
	_state->stop(*this);
}
void ALSource::update() {
	static int count = 0;
	++count;
	_state->update(*this);
	int state;
	ALECA(alGetSourcei, _id, AL_SOURCE_STATE, &state);
	if(state == AL_STOPPED)
		stop();
}
AState ALSource::getState() const {
	return _state->getState();
}
void ALSource::setLooping(bool bLoop) const {
	ALECA(alSourcei, _id, AL_LOOPING, bLoop ? AL_TRUE : AL_FALSE);
}
bool ALSource::isLooping() const {
	int val;
	ALECA(alGetSourcei, _id, AL_LOOPING, &val);
	return val == AL_TRUE;
}
void ALSource::setPitch(float pitch) {
	ALECA(alSourcef, _id, AL_PITCH, pitch);
}
void ALSource::setPosition(const Vec3& pos) {
	ALECA(alSource3f, _id, AL_POSITION, pos.x, pos.y, pos.z);
}
void ALSource::setDirection(const Vec3& dir) {
	ALECA(alSource3f, _id, AL_DIRECTION, dir.x, dir.y, dir.z);
}
void ALSource::setVelocity(const Vec3& vel) {
	ALECA(alSource3f, _id, AL_VELOCITY, vel.x, vel.y, vel.z);
}
void ALSource::setGain(float gain) {
	ALECA(alSourcef, _id, AL_GAIN, gain);
}
void ALSource::setGainRange(float gmin, float gmax) {
	ALECA(alSourcef, _id, AL_MIN_GAIN, gmin);
	ALECA(alSourcef, _id, AL_MAX_GAIN, gmax);
}
float ALSource::timeTell() const {
	float ret;
	ALECA(alGetSourcef, _id, AL_SEC_OFFSET, &ret);
	return ret;
}
int64_t ALSource::pcmTell() const {
	float ret;
	ALECA(alGetSourcef, _id, AL_SAMPLE_OFFSET, & ret);
	return static_cast<int64_t>(ret);
}
void ALSource::timeSeek(float t) const {
	ALECA(alSourcef, _id, AL_SEC_OFFSET, t);
}
void ALSource::pcmSeek(int64_t p) const {
	ALECA(alSourcef, _id, AL_SAMPLE_OFFSET, static_cast<float>(p));
}
void ALSource::setAngleGain(float inner, float outer) {
	ALECA(alSourcef, _id, AL_CONE_INNER_ANGLE, inner);
	ALECA(alSourcef, _id, AL_CONE_OUTER_ANGLE, outer);
}
void ALSource::setAngleOuterGain(float gain) {
	ALECA(alSourcef, _id, AL_CONE_OUTER_GAIN, gain);
}
ALuint ALSource::getID() const {
	return _id;
}
void ALSource::setBuffer(HAb hAb) {
	_state->setBuffer(*this, hAb);
}
