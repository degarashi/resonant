#include "sound.hpp"

namespace rs {
	Duration CalcTimeLength(int word_size, int ch, int hz, size_t buffLen) {
		auto dur = double(buffLen) / double(word_size * ch * hz);
		return std::chrono::microseconds(static_cast<uint64_t>(dur * 1000000));
	}
	uint64_t CalcSampleLength(int word_size, int ch, int hz, Duration dur) {
		using namespace std::chrono;
		auto bsize = word_size * ch;
		auto dd = static_cast<double>(duration_cast<microseconds>(dur).count()) / 1000000.0;
		dd *= double(bsize * hz);
		// キリの良い値に切り下げる
		return static_cast<uint64_t>(dd) / bsize * bsize;
	}
	uint64_t CalcSampleLength(const AFormatF& af, Duration dur) {
		return CalcSampleLength(af.getBitNum()/8, af.getChannels(), af.freq, dur);
	}

	ABuffer::ABuffer(): _format(AFormat::Format::Invalid, 0) {}
	// --------------------- ABufSub ---------------------
	ABufSub::ABufSub(HAb hAb): _hlAb(hAb) {
		auto& ab = hAb.ref();
		if(ab->isStreaming()) {
			_abuff.reset(new ABufferDep[MAX_AUDIO_BLOCKNUM]);
			_nBuffer = MAX_AUDIO_BLOCKNUM;
		} else {
			_abuff.reset(new ABufferDep[1]);
			_nBuffer = 1;
		}
		_offset = 0;
		_readCur = _playedCur = _writeCur = 0;
	}
	ABufSub::ABufSub(ABufSub&& a): _hlAb(std::move(a._hlAb)), _abuff(std::move(a._abuff)),
		_nBuffer(a._nBuffer), _readCur(a._readCur), _playedCur(a._playedCur), _writeCur(a._writeCur),
		_offset(a._offset)
	{}
	const AFormatF& ABufSub::getFormat() const {
		return _hlAb.cref()->getFormat();
	}
	Duration ABufSub::getDuration() const {
		return _hlAb.cref()->getDuration();
	}
	void ABufSub::_fillBuffer() {
		auto& ab = _hlAb.ref();
		if(_hlAb.cref()->isStreaming()) {
			constexpr int BUFFERSIZE = 8192*4;
			uint8_t buff[BUFFERSIZE];
			while(_writeCur < _playedCur + _nBuffer) {
				auto nread = ab->getData(buff, _offset, sizeof(buff));
				if(nread == 0)
					break;
				_offset += nread;
				_abuff[_writeCur % _nBuffer].writeBuffer(ab->getFormat(), buff, nread);
				++_writeCur;
			}
		} else {
			if(_writeCur != 1) {
				auto res = ab->getAllData(_offset);
				_abuff[0].writeBuffer(ab->getFormat(), res.first, res.second);
				++_writeCur;
			}
		}
	}
	ABufferDep* ABufSub::getBlock() {
		_fillBuffer();
		if(_readCur == _writeCur)
			return nullptr;
		ABufferDep* ret = &_abuff[_readCur % _nBuffer];
		++_readCur;
		return ret;
	}
	void ABufSub::rewind() {
		_readCur = _writeCur = _playedCur = 0;
		_offset = 0;
	}
	bool ABufSub::isEOF() {
		_fillBuffer();
		return _playedCur == _writeCur;
	}
	void ABufSub::setPlayedCursor(int cur) {
		Assert(Trap, _playedCur <= cur);
		_playedCur = cur;
	}
	void ABufSub::timeSeek(Duration t) {
		_readCur = _writeCur = _playedCur = 0;
		const AFormatF& af = _hlAb.ref()->getFormat();
		_offset = CalcSampleLength(af, t);
	}
	void ABufSub::pcmSeek(uint64_t t) {
		_readCur = _writeCur = _playedCur = 0;
		const AFormatF& af = _hlAb.ref()->getFormat();
		_offset = af.getBlockSize() * t;
	}

	// --------------------- AWaveBatch ---------------------
	struct SDLMem {
		Uint8* ptr = nullptr;
		~SDLMem() {
			if(ptr)
				SDL_free(ptr);
		}
	};
	AWaveBatch::AWaveBatch(HRW hRW) {
		auto& rw = hRW.ref();
		SDL_AudioSpec spec;
		SDLMem buff;
		Uint32 buffLen;
		SDLEC(Trap, SDL_LoadWAV_RW, hRW.ref().getOps(), 0, &spec, &buff.ptr, &buffLen);
		AFormat fmt(SDL_AUDIO_BITSIZE(spec.format) > 8, spec.channels!=1);
		_format = AFormatF(fmt, spec.freq);
		_buff.resize(buffLen);
		std::memcpy(&_buff[0], buff.ptr, buffLen);
		_duration = CalcTimeLength(fmt.getBitNum()/8, fmt.getChannels(), _format.freq, buffLen);
	}
	bool AWaveBatch::isStreaming() const { return false; }
	std::pair<const void*, size_t> AWaveBatch::getAllData(uint64_t offset) const {
		return std::make_pair(&_buff[offset], _buff.size()-offset);
	}

	// --------------------- AOggBatch ---------------------
	AOggBatch::AOggBatch(HRW hRW) {
		RawData rd = VorbisFile::ReadAll(hRW);
		_format = rd.format;
		_buff = std::move(rd.buff);
		_duration = CalcTimeLength(_format.getBitNum()/8, _format.getChannels(), _format.freq, rd.buff.size());
	}
	bool AOggBatch::isStreaming() const { return false; }
	std::pair<const void*, size_t> AOggBatch::getAllData(uint64_t offset) const {
		return std::make_pair(&_buff[offset], _buff.size()-offset);
	}

	// --------------------- AOggStream ---------------------
	AOggStream::AOggStream(HRW hRW): _vfile(hRW) {
		_format = _vfile.getFormat();
		_prevOffset = ~0;
		_duration = std::chrono::milliseconds(static_cast<uint32_t>(_vfile.timeLength() * 1000.f));
	}
	bool AOggStream::isStreaming() const { return true; }
	size_t AOggStream::getData(void* dst, uint64_t offset, size_t buffLen) const {
		uint64_t bsize = _format.getBlockSize();
		Assert(Trap, offset % bsize == 0);
		if(_prevOffset != offset) {
			auto pcmOffset = offset / bsize;
			_vfile.pcmSeek(pcmOffset);
		}

		size_t nread = _vfile.read(dst, buffLen);
		_prevOffset = offset + nread;
		return nread;
	}

	// --------------------- ASource ---------------------
	ASource::ASource(): _state(new S_Empty(*this)) {}
	ASource::ASource(ASource&& s): _dep(std::move(s._dep)),
		_state(std::move(s._state)), _opBuf(std::move(s._opBuf)), _nLoop(s._nLoop), _timePos(s._timePos), _pcmPos(s._pcmPos),
		_fadeTo(s._fadeTo), _fadeOut(s._fadeOut) {}
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
	void ASource::setGain(float gain) { _targetGain = gain; }
	AState ASource::getState() const { return _state->getState(); }
	void ASource::setRelativeMode(bool bRel) { _dep.setRelativeMode(bRel); }
	void ASource::setPosition(const Vec3& pos) { _dep.setPosition(pos); }
	void ASource::setDirection(const Vec3& dir) { _dep.setDirection(dir); }
	void ASource::setVelocity(const Vec3& vel) { _dep.setVelocity(vel); }
	void ASource::setGainRange(float gmin, float gmax) { _dep.setGainRange(gmin, gmax); }
	void ASource::setAngleGain(float inner, float outer) { _dep.setAngleGain(inner, outer); }
	void ASource::setAngleOuterGain(float gain) { _dep.setAngleOuterGain(gain); }
	uint32_t ASource::getLooping() const { return _nLoop; }
	Duration ASource::timeTell() {
	// 	return _dep.timeTell(_timePos);
		return _timePos;
	}
	uint64_t ASource::pcmTell() {
	// 	return _dep.pcmTell(_pcmPos);
		return _pcmPos;
	}
	void ASource::timeSeek(Duration t) { _state->timeSeek(*this, t); }
	void ASource::pcmSeek(uint64_t p) { _state->pcmSeek(*this, p); }
	void ASource::setBuffer(HAb hAb, Duration fadeIn, uint32_t nLoop) {
		_nLoop = nLoop;
		_duration = hAb.ref()->getDuration() * (nLoop+1);
		_fadeTo.init(std::chrono::seconds(0), fadeIn, 1.f);
		_fadeOut.init();
		_currentGain = _targetGain = 0.f;
		_dep.setGain(0);
		_state->setBuffer(*this, hAb);
	}
	void ASource::fadeTo(float gain, Duration dur) {
		_fadeTo.init(_timePos, dur, gain);
	}
	void ASource::setFadeOut(Duration dur, bool bNow) {
		if(bNow)
			_fadeOut.init(_timePos, dur, 0.f);
		else
			_fadeOut.init(_duration - dur, dur, 0.f);
	}
	void ASource::_applyFades() {
		float f;
		float gain = _targetGain;
		if(_fadeTo.apply(*this, f))
			gain = f;
		if(_fadeOut.apply(*this, f))
			gain = f;
		_targetGain = gain;
	}
	void ASource::_advanceGain() {
		_currentGain += (_targetGain - _currentGain)*0.75f;
		_dep.setGain(_currentGain);
	}
	void ASource::Fade::init() { bValid = false; }
	void ASource::Fade::init(Duration cur, Duration dur, float to) {
		bValid = true;
		durFadeBegin = cur;
		durFadeEnd = cur + dur;
		fromGain = -1;
		toGain = to;
	}
	bool ASource::Fade::apply(ASource& self, float& gain) {
		if(bValid) {
			Duration tp = self._timePos;
			if(durFadeBegin <= tp) {
				if(durFadeEnd <= tp) {
					gain = toGain;
					bValid = false;
				} else {
					if(fromGain < 0)
						fromGain = self._currentGain;

					using namespace std::chrono;
					float r = float(duration_cast<microseconds>(tp - durFadeBegin).count()) / float(duration_cast<microseconds>(durFadeEnd - durFadeBegin).count());
					gain = (toGain-fromGain) * r + fromGain;
				}
				return true;
			}
		}
		return false;
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
		self._timePos = std::chrono::seconds(0);
		self._playedCur = 0;
		self._opBuf->rewind();
	}
	void ASource::_refillBuffer() {
		_dep.clearBlock();
		// 最初のキューの準備
		int num = MAX_AUDIO_BLOCKNUM;
		while(--num >= 0) {
			auto* pb = _opBuf->getBlock();
			if(!pb)
				break;
			_dep.enqueue(*pb);
		}
	}
	void ASource::S_Initial::play(ASource& self) {
		self._refillBuffer();
		self._setState<S_Playing>();
	}
	void ASource::S_Playing::pcmSeek(ASource& self, uint64_t t) {}
	void ASource::S_Playing::update(ASource& self) {
		self._applyFades();
		// 前回のUpdateを呼んだ時間からの差を累積
		Timepoint tnow = Clock::now();
		Duration dur = tnow - self._tmUpdate;
		self._tmUpdate = tnow;
		self._timePos += dur;
		// 再生時間からサンプル数を計算
		const AFormatF& af = self._opBuf->getFormat();
		self._pcmPos = CalcSampleLength(af, self._timePos);

		// 終了チェック
		if(self._opBuf->isEOF()) {
			// ループ回数が残っていればもう一周
			if(self._nLoop-- == 0) {
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
				self._playedCur += nproc;
				self._opBuf->setPlayedCursor(self._playedCur);
				ABufferDep* pb;
				while(--nproc >= 0 && (pb = self._opBuf->getBlock()))
					self._dep.enqueue(*pb);
			}
			self._dep.update(!self._opBuf->isEOF());
			IState::update(self);
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
		return fadeIn(hAb, std::chrono::seconds(0), nLoop);
	}
	HSs AGroup::fadeIn(HAb hAb, Duration fadeIn, int nLoop) {
		return fadeInOut(hAb, fadeIn, std::chrono::seconds(0), nLoop);
	}
	HSs AGroup::fadeInOut(HAb hAb, Duration fadeIn, Duration fadeOut, int nLoop) {
		if(getIdleChannels() > 0) {
			for(auto& s : _source) {
				auto& ch = s.ref();
				auto st = ch.getState();
				if(st != AState::Playing &&
					st != AState::Paused)
				{
					++_nActive;
					ch.setBuffer(hAb, fadeIn, nLoop);
					if(fadeOut > std::chrono::seconds(0))
						ch.setFadeOut(fadeOut, false);
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
}
