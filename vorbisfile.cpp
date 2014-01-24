#include "sound_common.hpp"

namespace rs {
	size_t VorbisFile::ReadOGC(void* ptr, size_t blocksize, size_t nmblock, void* datasource) {
		auto* ops = reinterpret_cast<SDL_RWops*>(datasource);
		Sint64 fpos = SDL_RWseek(ops, 0, RW_SEEK_CUR);
		Sint64 fsize = SDL_RWseek(ops, 0, RW_SEEK_END);
		SDL_RWseek(ops, fpos, RW_SEEK_SET);

		size_t nblock = std::min((fsize-fpos)/blocksize, Sint64(nmblock));
		return SDL_RWread(ops, ptr, blocksize, nblock);
	}
	int VorbisFile::SeekOGC(void* datasource, ogg_int64_t offset, int whence) {
		auto* ops = reinterpret_cast<SDL_RWops*>(datasource);
		if(!ops)
			return -1;
		if(whence == SEEK_SET)
			whence = RW_SEEK_SET;
		else if(whence == SEEK_CUR)
			whence = RW_SEEK_CUR;
		else if(whence == SEEK_END)
			whence = RW_SEEK_END;
		return SDL_RWseek(ops, offset, whence);
	}
	int VorbisFile::CloseOGC(void* datasource) {
		auto* ops = reinterpret_cast<SDL_RWops*>(datasource);
		if(!ops)
			return EOF;

		SDL_RWclose(ops);
		return 0;
	}
	long VorbisFile::TellOGC(void* datasource) {
		auto* ops = reinterpret_cast<SDL_RWops*>(datasource);
		if(!ops)
			return -1;

		return SDL_RWtell(ops);
	}

	ov_callbacks VorbisFile::OVCallbacksNF = {
		ReadOGC, SeekOGC, nullptr, TellOGC
	};
	ov_callbacks VorbisFile::OVCallbacks = {
		ReadOGC, SeekOGC, CloseOGC, TellOGC
	};
	RawData VorbisFile::ReadAll(HRW hRW) {
		VorbisFile vf(hRW);

		auto totalbyte = vf.getFormat().getBlockSize() * vf.pcmLength();
		RawData rd(totalbyte);
		size_t nread = vf.read(&rd.buff[0], totalbyte);
		rd.buff.resize(nread);
		rd.format = vf.getFormat();
		return std::move(rd);
	}
	void VorbisFile::_init() {
		auto& rw = _hlRW.ref();
		rw.seek(_initialFPos, RWops::Begin);
		OVEC_P(Trap, ov_open_callbacks, rw.getOps(), &_ovf, nullptr, 0, OVCallbacksNF);
		vorbis_info* info = ov_info(&_ovf, -1);
		// Oggのフォーマットは全てsigned int 16bitとみなす
		_format = AFormatF(AFormat(true, info->channels > 1), info->rate);
		_dTotal = OVEC_P(Trap, ov_time_total, &_ovf, -1);
		_iTotal = OVEC_P(Trap, ov_pcm_total, &_ovf, -1);
	}
	VorbisFile::VorbisFile(HRW hRW) {
		_hlRW = hRW;
		_initialFPos = hRW.ref().tell();
		_init();
	}
	VorbisFile::~VorbisFile() {
		OVEC_P(Trap, ov_clear, &_ovf);
	}
	const AFormatF& VorbisFile::getFormat() const {
		return _format;
	}

	bool VorbisFile::isEOF() const {
		return pcmTell() == _iTotal;
	}
	bool VorbisFile::timeSeek(double s) {
		OVEC_P(Trap, ov_time_seek, &_ovf, s);
		return isEOF();
	}
	void VorbisFile::timeSeekPage(double s) {
		OVEC_P(Trap, ov_time_seek_page, &_ovf, s);
	}
	bool VorbisFile::timeSeekLap(double s) {
		OVEC_P(Trap, ov_time_seek_lap, &_ovf, s);
		return isEOF();
	}
	void VorbisFile::timeSeekPageLap(double s) {
		OVEC_P(Trap, ov_time_seek_page_lap, &_ovf, s);
	}
	bool VorbisFile::pcmSeek(int64_t pos) {
		OVEC_P(Trap, ov_pcm_seek, &_ovf, pos);
		return isEOF();
	}
	void VorbisFile::pcmSeekPage(int64_t pos) {
		OVEC_P(Trap, ov_pcm_seek_page, &_ovf, pos);
	}
	bool VorbisFile::pcmSeekLap(int64_t pos) {
		OVEC_P(Trap, ov_pcm_seek_lap, &_ovf, pos);
		return isEOF();
	}
	void VorbisFile::pcmSeekPageLap(int64_t pos) {
		OVEC_P(Trap, ov_pcm_seek_page_lap, &_ovf, pos);
	}
	size_t VorbisFile::read(void* dst, size_t toRead) {
		if(toRead == 0)
			return 0;

		auto pDst = reinterpret_cast<char*>(dst);
		size_t nRead;
		int bs;
		do {
			nRead = ov_read(&_ovf, pDst, toRead, 0, 2, 1, &bs);
			if(nRead == 0)
				break;
			toRead -= nRead;
			pDst += nRead;
		} while(toRead > 0);
		return pDst - reinterpret_cast<char*>(dst);
	}
	double VorbisFile::timeLength() const {
		return _dTotal;
	}
	int64_t VorbisFile::pcmLength() const {
		return _iTotal;
	}
	double VorbisFile::timeTell() const {
		return OVEC_P(Trap, ov_time_tell, const_cast<OggVorbis_File*>(&this->_ovf));
	}
	int64_t VorbisFile::pcmTell() const {
		auto* self = const_cast<VorbisFile*>(this);
		return OVEC_P(Trap, ov_pcm_tell, const_cast<OggVorbis_File*>(&this->_ovf));
	}
	void VorbisFile::invalidate() {
		_hlRW.setNull();
	}
}
