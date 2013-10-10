#include "sound.hpp"

ABuffer::ABuffer(): _format(AFormat::Format::Invalid, 0) {}

// --------------------- AWaveBatch ---------------------
struct SDLMem {
	Uint8* ptr = nullptr;
	~SDLMem() {
		if(ptr)
			SDL_free(ptr);
	}
};
AWaveBatch::AWaveBatch(sdlw::HRW hRW): _bUsed(false) {
	auto& rw = hRW.ref();
	SDL_AudioSpec spec;
	SDLMem buff;
	Uint32 buffLen;
	SDLEC(SDL_LoadWAV_RW, hRW.ref().getOps(), 0, &spec, &buff.ptr, &buffLen);
	AFormat fmt(SDL_AUDIO_BITSIZE(spec.format) > 8, spec.channels!=1);
	_format = AFormatF(fmt, spec.freq);
	_abuff.writeBuffer(_format, buff.ptr, buffLen);
}
ABufferDep* AWaveBatch::getBlock() {
	if(_bUsed)
		return nullptr;
	_bUsed = true;
	return &_abuff;
}
void AWaveBatch::rewind() {
	_bUsed = false;
}
void AWaveBatch::timeSeek(float t) {}
void AWaveBatch::pcmSeek(int64_t t) {}

// --------------------- AOggBatch ---------------------
AOggBatch::AOggBatch(sdlw::HRW hRW) {
	RawData rd = VorbisFile::ReadAll(hRW);
	_format = rd.format;
	_abuff.writeBuffer(rd.format, &rd.buff[0], rd.buff.size());
}
ABufferDep* AOggBatch::getBlock() {
	if(_bUsed)
		return nullptr;
	_bUsed = true;
	return &_abuff;
}
void AOggBatch::rewind() {
	_bUsed = false;
}
void AOggBatch::timeSeek(float t) {}
void AOggBatch::pcmSeek(int64_t t) {}

// --------------------- AOggStream ---------------------
AOggStream::AOggStream(sdlw::HRW hRW): _vfile(hRW) {
	_format = _vfile.getFormat();
	rewind();
}
void AOggStream::_fillBuffer() {
	uint8_t buff[32768];
	while(_nFree > 0) {
		--_nFree;

		size_t nread = _vfile.read(buff, sizeof(buff));
		if(nread == 0)
			break;
		int idx = _writeCur % MAX_AUDIO_BLOCKNUM;
		_abuff[idx].writeBuffer(_format, buff, nread);
		++_writeCur;
	}
}
void AOggStream::setUsedBlock(int n) {
	_nFree += n;
	Assert(_nFree <= MAX_AUDIO_BLOCKNUM);
}
ABufferDep* AOggStream::getBlock() {
	_fillBuffer();
	if(_readCur == _writeCur)
		return nullptr;
	ABufferDep* ret = _abuff + (_readCur % MAX_AUDIO_BLOCKNUM);
	++_readCur;
	return ret;
}
void AOggStream::rewind() {
	_readCur = _writeCur = 0;
	_nFree = MAX_AUDIO_BLOCKNUM;
	_vfile.pcmSeekPage(0);
	_fillBuffer();
}
void AOggStream::timeSeek(float t) {}
void AOggStream::pcmSeek(int64_t t) {}

// --------------------- ASource ---------------------
ASource::ASource(): _state(new S_Empty(*this)) {}
ASource::ASource(ASource&& s): _dep(std::move(s._dep)),
	_state(std::move(s._state)), _hlAb(std::move(s._hlAb)), _nLoop(s._nLoop), _tmBegin(s._tmBegin), _timePos(s._timePos), _pcmPos(s._pcmPos) {}
ASource::~ASource() {
	if(_state)
		stop();
}
void ASource::play() { _state->play(*this); }
void ASource::pause() { _state->pause(*this); }
void ASource::rewind() { _state->rewind(*this); }
void ASource::stop() { _state->stop(*this); }
void ASource::update() { _state->update(*this); }
void ASource::setPitch(float pitch) { _dep.setPitch(pitch); }
void ASource::setGain(float gain) { _dep.setGain(gain); }
AState ASource::getState() const { return _state->getState(); }
void ASource::setRelativeMode(bool bRel) { _dep.setRelativeMode(bRel); }
void ASource::setPosition(const Vec3& pos) { _dep.setPosition(pos); }
void ASource::setDirection(const Vec3& dir) { _dep.setDirection(dir); }
void ASource::setVelocity(const Vec3& vel) { _dep.setVelocity(vel); }
void ASource::setGainRange(float gmin, float gmax) { _dep.setGainRange(gmin, gmax); }
void ASource::setAngleGain(float inner, float outer) { _dep.setAngleGain(inner, outer); }
void ASource::setAngleOuterGain(float gain) { _dep.setAngleOuterGain(gain); }
int ASource::getLooping() const { return _nLoop; }
float ASource::timeTell() { return _dep.timeTell(_timePos); }
int64_t ASource::pcmTell() { return _dep.pcmTell(_pcmPos); }
void ASource::timeSeek(float t) { _state->timeSeek(*this, t); }
void ASource::pcmSeek(int64_t p) { _state->pcmSeek(*this, p); }
void ASource::setBuffer(HAb hAb, int nLoop) {
	_nLoop = nLoop;
	_state->setBuffer(*this, hAb);
}
// --------------------- ASource::IState ---------------------
void ASource::IState::setBuffer(ASource& self, HAb hAb) {
	stop(self);
	self._setState<S_Initial>(hAb);
}
void ASource::S_Initial::_init(ASource& self) {
	self._dep.reset();
	self._dep.clearBlock();
	self._pcmPos = 0;
	self._timePos = 0;
	self._hlAb.ref()->rewind();
}
void ASource::S_Playing::timeSeek(ASource& self, float t) {
	// 一旦止めてキューにセットし直す
	self._setState<S_Paused>();
	self._dep.clearBlock();
	auto& buff = self._hlAb.ref();
	buff->timeSeek(t);
	int num = MAX_AUDIO_BLOCKNUM;
	while(--num >= 0) {
		auto* pb = buff->getBlock();
		if(!pb)
			break;
		self._dep.enqueue(*pb);
	}
}
void ASource::S_Playing::pcmSeek(ASource& self, int64_t t) {
}
void ASource::S_Initial::play(ASource& self) {
	self._tmBegin = Clock::now();
	// 最初のキューの準備
	auto& buff = self._hlAb.ref();
	int num = MAX_AUDIO_BLOCKNUM;
	while(--num >= 0) {
		auto* pb = buff->getBlock();
		if(!pb)
			break;
		self._dep.enqueue(*pb);
	}
	self._setState<S_Playing>();
}
void ASource::S_Playing::update(ASource& self) {
	// 終了チェック
	if(self._dep.update()) {
		// ループ回数が残っていればもう一周
		if(--self._nLoop < 0) {
			// そうでなければ終了ステートへ
			self._setState<S_Stopped>();
		} else {
			self._setState<S_Initial>();
			self.play();
		}
	} else {
		// キューの更新
		int nproc = self._dep.getUsedBlock();
		if(nproc > 0) {
			auto& buff = self._hlAb.ref();
			buff->setUsedBlock(nproc);
			ABufferDep* pb;
			while(--nproc >= 0 && (pb = buff->getBlock()))
				self._dep.enqueue(*pb);
		}
	}
}

// ------------------ ALGroup ------------------
AGroup::AGroup(AGroup&& a): _source(std::move(a._source)), _nActive(a._nActive), _bPaused(a._bPaused) {}
AGroup::AGroup(int n): _source(n), _nActive(0), _bPaused(false) {
	for(auto& s : _source)
		s = mgr_sound.createSource();
}
void AGroup::update() {
	int na = 0;
	for(auto& s : _source) {
		auto st = s.ref().getState();
		if(st == AState::Playing ||
			st == AState::Paused)
			++na;
	}
	_nActive = na;
}
void AGroup::pause() {
	if(!_bPaused) {
		_bPaused = true;
		for(auto& s : _source)
			s.ref().pause();
	}
}
void AGroup::resume() {
	if(_bPaused) {
		_bPaused = false;
		for(auto& s : _source)
			s.ref().play();
	}
}
void AGroup::clear() {
	for(auto& s : _source)
		s.ref().stop();
	_nActive = getChannels();
	_bPaused = false;
}
HSs AGroup::play(HAb hAb, int nLoop) {
	if(getIdleChannels() > 0) {
		for(auto& s : _source) {
			auto& ch = s.ref();
			auto st = ch.getState();
			if(st != AState::Playing &&
				st != AState::Paused)
			{
				++_nActive;
				ch.setBuffer(hAb, nLoop);
				if(!_bPaused)
					ch.play();
				return s.get();
			}
		}
	}
	return HSs();
}

int AGroup::getChannels() const { return static_cast<int>(_source.size()); }
int AGroup::getIdleChannels() const { return getChannels() - _nActive; }
int AGroup::getPlayingChannels() const { return _nActive; }

// ------------------ SoundMgr ------------------
HLAb SoundMgr::loadWaveBatch(HRW hRw) {
	return _buffMgr.acquire(UPABuff(new AWaveBatch(hRw)));
}
HLAb SoundMgr::loadOggBatch(HRW hRw) {
	return _buffMgr.acquire(UPABuff(new AOggBatch(hRw)));
}
HLAb SoundMgr::loadOggStream(HRW hRw) {
	return _buffMgr.acquire(UPABuff(new AOggStream(hRw)));
}

HLSg SoundMgr::createSourceGroup(int n) {
	return _sgMgr.acquire(n);
}
HLSs SoundMgr::createSource() {
	return _srcMgr.acquire(ASource());
}
void SoundMgr::update() {
	for(auto& s : _srcMgr)
		s.update();
	for(auto& sg : _sgMgr)
		sg.update();
}
