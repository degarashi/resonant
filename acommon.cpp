#include "sound_common.hpp"

namespace rs {
	std::ostream& operator << (std::ostream& os, const Duration& d) {
		return os << "(duration)" << std::chrono::duration_cast<Microseconds>(d).count() << " microsec";
	}
	std::ostream& operator << (std::ostream& os, const Timepoint& t) {
		return os << "(timepoint)" << std::chrono::duration_cast<Milliseconds>(t.time_since_epoch()).count() << " ms";
	}

	// --------------------- OVError ---------------------
	const char* OVError::getAPIName() const {
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
	const char* OVError::errorDesc(int err) const {
		if(err < 0) {
			for(auto& p : ErrorList) {
				if(p.first == err)
					return p.second;
			}
			return "unknown error";
		}
		return nullptr;
	}
	// --------------------- RawData ---------------------
	RawData::RawData(size_t sz): buff(sz) {}
	RawData::RawData(RawData&& rd): format(rd.format), buff(std::move(rd.buff)) {}

	// --------------------- AFormat ---------------------
	AFormat::AFormat(Format fmt): format(fmt) {}
	AFormat::AFormat(SDLAFormat f, bool bStereo): AFormat(f.getBitSize()>8, bStereo) {}
	AFormat::AFormat(bool b16Bit, bool bStereo) {
		if(b16Bit)
			format = bStereo ? Format::Stereo16 : Format::Mono16;
		else
			format = bStereo ? Format::Stereo8 : Format::Mono8;
	}

	int AFormat::getBitNum() const {
		if(format==Format::Mono8 || format==Format::Stereo8)
			return 8;
		return 16;
	}
	int AFormat::getChannels() const {
		if(format==Format::Mono8 || format==Format::Mono16)
			return 1;
		return 2;
	}
	size_t AFormat::getBlockSize() const {
		return getBitNum()/8 * getChannels();
	}
	// --------------------- AFormatF ---------------------
	AFormatF::AFormatF(AFormat fmt, int fr): AFormat(fmt), freq(fr) {}

	// --------------------- SDLAFormat ---------------------
	SDLAFormat::SDLAFormat(SDL_AudioFormat fmt): format(fmt) {}
	SDLAFormat::SDLAFormat(const AFormat& afmt): SDLAFormat(1, 0, 0, afmt.getBitNum()) {}
	SDLAFormat::SDLAFormat(int issigned, int isbig, int isfloat, int bitsize) {
		format = bitsize & SDL_AUDIO_MASK_BITSIZE;
		format |= (isfloat & 1) << 8;
		format |= (isbig & 1) << 12;
		format |= (issigned & 1) << 15;
	}
	bool SDLAFormat::isSigned() const {
		return SDL_AUDIO_ISSIGNED(format);
	}
	bool SDLAFormat::isBigEndian() const {
		return SDL_AUDIO_ISBIGENDIAN(format);
	}
	bool SDLAFormat::isFloat() const {
		return SDL_AUDIO_ISFLOAT(format);
	}
	size_t SDLAFormat::getBitSize() const {
		return SDL_AUDIO_BITSIZE(format);
	}
	// --------------------- SDLAFormatCF ---------------------
	SDLAFormatCF::SDLAFormatCF(SDLAFormat fmt, int fr): SDLAFormat(fmt), channels(fmt.getBitSize()==8 ? 1 : 2), freq(fr) {}
	SDLAFormatCF::SDLAFormatCF(const AFormatF& af): SDLAFormat(static_cast<AFormat>(af)), channels(af.getChannels()), freq(af.freq) {}
}
