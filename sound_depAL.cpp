#include "sound_depAL.hpp"

namespace {
	const ALenum c_afmt[] = {
		AL_FORMAT_MONO8,
		AL_FORMAT_MONO16,
		AL_FORMAT_STEREO8,
		AL_FORMAT_STEREO16,
		-1
	};
}
ALenum AsALFormat(const AFormat& f) {
	return c_afmt[static_cast<int>(f.format)];
}
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

// --------------------- ABuffer_depAL ---------------------
ABuffer_depAL::ABuffer_depAL() {
	ALEC(alGenBuffers, 1, &_id);
}
ABuffer_depAL::~ABuffer_depAL() {
	ALEC(alDeleteBuffers, 1, &_id);
}
ALuint ABuffer_depAL::getID() const {
	return _id;
}
void ABuffer_depAL::writeBuffer(const AFormatF& af, const void* src, size_t len) {
	ALECA(alBufferData, _id, AsALFormat(af), src, len, af.freq);
}
// void ABuffer_depAL::atState(ASource_depAL& src, AState state) {
// 	if(state == AState::Initial)
// 		ALECA(alSourcei, src.getID(), AL_BUFFER, getID());
// }
// キューのプッシュと残数確認、終了チェック

// --------------------- ASource_depAL ---------------------
ASource_depAL::ASource_depAL() {
	ALEC(alGenSources, 1, &_id);
}
ASource_depAL::ASource_depAL(ASource_depAL&& a): _id(a._id) {
	a._id = 0;
}
ASource_depAL::~ASource_depAL() {
	if(_id != 0) {
		ALECA(alSourceStop, _id);
		ALECA(alDeleteSources, 1, &_id);
	}
}
void ASource_depAL::play() { ALECA(alSourcePlay, _id); }
void ASource_depAL::reset() {
	ALECA(alSourceStop, _id);
	ALECA(alSourceRewind, _id);
}
void ASource_depAL::pause() { ALECA(alSourcePause, _id); }
bool ASource_depAL::update() {
	// 終了判定
	ALint state;
	ALECA(alGetSourcei, _id, AL_SOURCE_STATE, &state);
	if(state == AL_STOPPED)
		return true;
	return false;
}
void ASource_depAL::setGain(float vol) {
	ALECA(alSourcef, _id, AL_GAIN, vol);
}
void ASource_depAL::setPitch(float pitch) {
	ALECA(alSourcef, _id, AL_PITCH, pitch);
}
float ASource_depAL::timeTell(float def) {
	float ret;
	ALECA(alGetSourcef, _id, AL_SEC_OFFSET, &ret);
	return ret;
}
int64_t ASource_depAL::pcmTell(int64_t def) {
	float ret;
	ALECA(alGetSourcef, _id, AL_SAMPLE_OFFSET, & ret);
	return static_cast<int64_t>(ret);
}
void ASource_depAL::timeSeek(float t) {
	ALECA(alSourcef, _id, AL_SEC_OFFSET, t);
}
void ASource_depAL::pcmSeek(int64_t t) {
	// floatでは精度が不足するかもしれないが、int32_tだと桁が足りないかもしれない
	ALECA(alSourcef, _id, AL_SAMPLE_OFFSET, static_cast<ALfloat>(t));
}
void ASource_depAL::enqueue(ABuffer_depAL& buff) {
	ALuint buffID = buff.getID();
	ALECA(alSourceQueueBuffers, _id, 1, &buffID);
}
int ASource_depAL::getUsedBlock() {
	int nproc;
	ALECA(alGetSourcei, _id, AL_BUFFERS_PROCESSED, &nproc);
	if(nproc > 0) {
		ALuint ids[MAX_AUDIO_BLOCKNUM];
		ALECA(alSourceUnqueueBuffers, _id, nproc, ids);
	}
	return nproc;
}
void ASource_depAL::clearBlock() {
	ALECA(alSourcei, _id, AL_BUFFER, 0);
}
ALuint ASource_depAL::getID() const { return _id; }

void ASource_depAL::setRelativeMode(bool bRel) {
	ALECA(alSourcei, _id, AL_SOURCE_RELATIVE, bRel ? AL_TRUE : AL_FALSE);
}
void ASource_depAL::setPosition(const Vec3& pos) {
	ALECA(alSource3f, _id, AL_POSITION, pos.x, pos.y, pos.z);
}
void ASource_depAL::setDirection(const Vec3& dir) {
	ALECA(alSource3f, _id, AL_DIRECTION, dir.x, dir.y, dir.z);
}
void ASource_depAL::setVelocity(const Vec3& vel) {
	ALECA(alSource3f, _id, AL_VELOCITY, vel.x, vel.y, vel.z);
}
void ASource_depAL::setGainRange(float gmin, float gmax) {
	ALECA(alSourcef, _id, AL_MIN_GAIN, gmin);
	ALECA(alSourcef, _id, AL_MAX_GAIN, gmax);
}
void ASource_depAL::setAngleGain(float inner, float outer) {
	ALECA(alSourcef, _id, AL_CONE_INNER_ANGLE, inner);
	ALECA(alSourcef, _id, AL_CONE_OUTER_ANGLE, outer);
}
void ASource_depAL::setAngleOuterGain(float gain) {
	ALECA(alSourcef, _id, AL_CONE_OUTER_GAIN, gain);
}

// --------------------- SoundMgr_depAL ---------------------
SoundMgr_depAL::SoundMgr_depAL(int rate): _rate(rate) {
	_device = alcOpenDevice(nullptr);
	if(!_device)
		throw std::runtime_error("");
}
SoundMgr_depAL::~SoundMgr_depAL() {
	for(auto* p : _context) {
		if(p)
			ALCEC(alcDestroyContext, p);
	}
	alcCloseDevice(_device);
}
ALCdevice* SoundMgr_depAL::getDevice() const { return _device; }
void SoundMgr_depAL::printVersions(std::ostream& os) {
	os << "version: " << alGetString(AL_VERSION) << std::endl
		<< "vendor: " << alGetString(AL_VENDOR) << std::endl
		<< "renderer: " << alGetString(AL_RENDERER) << std::endl
		<< "extensions: " << alGetString(AL_EXTENSIONS) << std::endl;
}
const std::pair<ALCenum, const char*> SoundMgr_depAL::ErrorList[] = {
	{ALC_NO_ERROR, "No error"},
	{ALC_INVALID_DEVICE, "Invalid device handle"},
	{ALC_INVALID_CONTEXT, "Invalid context handle"},
	{ALC_INVALID_ENUM, "Invalid enum parameter passed to an ALC call"},
	{ALC_INVALID_VALUE, "Invalid value parameter passed to an ALC call"},
	{ALC_OUT_OF_MEMORY, "Out of memory"}
};
const char* SoundMgr_depAL::GetAPIName() {
	return "OpenAL_C";
}
const char* SoundMgr_depAL::ErrorDesc() {
	ALCenum num = alcGetError(SoundMgr_depAL::_ref().getDevice());
	if(num != ALC_NO_ERROR) {
		for(auto& p : ErrorList) {
			if(p.first == num)
				return p.second;
		}
		return "unknown error";
	}
	return nullptr;
}
void SoundMgr_depAL::SetPosition(const Vec3& pos) {
	ALECA(alListener3f, AL_POSITION, pos.x, pos.y, pos.z);
}
void SoundMgr_depAL::SetVelocity(const Vec3& v) {
	ALECA(alListener3f, AL_VELOCITY, v.x, v.y, v.z);
}
void SoundMgr_depAL::SetGain(float g) {
	ALECA(alListenerf, AL_GAIN, g);
}
void SoundMgr_depAL::SetOrientation(const Vec3& dir, const Vec3& up) {
	const float tmp[] = {dir.x, dir.y, dir.z,
	up.x, up.y, up.z};
	ALECA(alListenerfv, AL_ORIENTATION, tmp);
}
void SoundMgr_depAL::SetDopplerFactor(float f) {
	// 	alGetFloat(AL_DOPPLER_FACTOR);
	ALECA(alDopplerFactor, f);
}
void SoundMgr_depAL::SetSpeedOfSound(float s) {
	// 	alGetFloat(AL_SPEED_OF_SOUND);
	ALECA(alSpeedOfSound, s);
}
void SoundMgr_depAL::SetDistModel(DistModel model) {
	// 	alGetInteger(AL_DISTANCE_MODEL);
	ALECA(alDistanceModel, static_cast<ALenum>(model));
}

SDL_atomic_t SoundMgr_depAL::s_atmThCounter = {};
sdlw::TLS<int> SoundMgr_depAL::s_thID = []() {
	return SDL_AtomicAdd(&s_atmThCounter, 0x01)+1;
}();

void SoundMgr_depAL::makeCurrent() {
	sdlw::UniLock lk(_mutex);
	int idx = s_thID.get();
	int sz = _context.size();
	if(sz <= idx)
		_context.resize(idx+1);
	if(!_context[idx]) {
		const ALint attr[] = {ALC_FREQUENCY, _rate, 0};
		_context[idx] = ALCEC(alcCreateContext, getDevice(), attr);
	}
	ALCECA(alcMakeContextCurrent, _context[idx]);
}
void SoundMgr_depAL::resetCurrent() {
	sdlw::UniLock lk(_mutex);
	ALCECA(alcMakeContextCurrent, nullptr);
}
void SoundMgr_depAL::suspend() {
	sdlw::UniLock lk(_mutex);
	ALCECA(alcSuspendContext, _context[s_thID.get()]);
}
void SoundMgr_depAL::process() {
	sdlw::UniLock lk(_mutex);
	ALCECA(alcProcessContext, _context[s_thID.get()]);
}
