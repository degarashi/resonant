#include "sound_common.hpp"

size_t VorbisFile::ReadOGC(void* ptr, size_t blocksize, size_t nmblock, void* datasource) {
	auto* ops = reinterpret_cast<SDL_RWops*>(datasource);
	int fpos = SDL_RWseek(ops, 0, RW_SEEK_CUR);
	int fsize = SDL_RWseek(ops, 0, RW_SEEK_END);
	SDL_RWseek(ops, fpos, RW_SEEK_SET);

	size_t nblock = std::min((fsize-fpos)/blocksize, nmblock);
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
RawData VorbisFile::ReadAll(sdlw::HRW hRW) {
	VorbisFile vf(hRW);

	auto totalbyte = vf.getFormat().getBlockSize() * vf.pcmLength();
	RawData rd(totalbyte);
	size_t nread = vf.read(&rd.buff[0], totalbyte);
	rd.buff.resize(nread);
	rd.format = vf.getFormat();
	return std::move(rd);
}
VorbisFile::VorbisFile(sdlw::HRW hRW) {
	_hlRW = hRW;

	OVECA(ov_open_callbacks, hRW.ref().getOps(), &_ovf, nullptr, 0, OVCallbacksNF);
	vorbis_info* info = ov_info(&_ovf, -1);
	// Oggのフォーマットは全てsigned int 16bitとみなす
	_format = AFormatF(AFormat(true, info->channels > 1), info->rate);
	_dTotal = OVECA(ov_time_total, &_ovf, -1);
	_iTotal = OVECA(ov_pcm_total, &_ovf, -1);
}
VorbisFile::~VorbisFile() {
	OVECA(ov_clear, &_ovf);
}
const AFormatF& VorbisFile::getFormat() const {
	return _format;
}

bool VorbisFile::isEOF() {
	return pcmTell() == _iTotal;
}
bool VorbisFile::timeSeek(double s) {
	OVECA(ov_time_seek, &_ovf, s);
	return isEOF();
}
void VorbisFile::timeSeekPage(double s) {
	OVECA(ov_time_seek_page, &_ovf, s);
}
bool VorbisFile::timeSeekLap(double s) {
	OVECA(ov_time_seek_lap, &_ovf, s);
	return isEOF();
}
void VorbisFile::timeSeekPageLap(double s) {
	OVECA(ov_time_seek_page_lap, &_ovf, s);
}
bool VorbisFile::pcmSeek(int64_t pos) {
	OVECA(ov_pcm_seek, &_ovf, pos);
	return isEOF();
}
void VorbisFile::pcmSeekPage(int64_t pos) {
	OVECA(ov_pcm_seek_page, &_ovf, pos);
}
bool VorbisFile::pcmSeekLap(int64_t pos) {
	OVECA(ov_pcm_seek_lap, &_ovf, pos);
	return isEOF();
}
void VorbisFile::pcmSeekPageLap(int64_t pos) {
	OVECA(ov_pcm_seek_page_lap, &_ovf, pos);
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
double VorbisFile::timeTell() {
	return OVECA(ov_time_tell, &_ovf);
}
int64_t VorbisFile::pcmTell() {
	return OVECA(ov_pcm_tell, &_ovf);
}
