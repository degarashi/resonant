#pragma once
#include <ogg/ogg.h>
#include <vorbis/vorbisfile.h>
#include "sdlwrap.hpp"
#include "spinner/vector.hpp"
#include "spinner/misc.hpp"
#include "clock.hpp"

#define OVEC(...) EChk_baseA1<true, OVError>(__FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__);
#ifdef DEBUG
	#define OVECA(...) OVEC(__VA_ARGS__)
#else
	#define OVECA(...) EChk_pass(__VA_ARGS__)
#endif

namespace rs {
	struct OVError {
		const static std::pair<int, const char*> ErrorList[];
		static const char* ErrorDesc(int err);
		static const char* GetAPIName();
	};
	enum class AState {
		Initial,
		Playing,
		Paused,
		Stopped,
		Empty
	};
	using spn::Vec3;
	struct AFormat;
	struct AFormatF;
	struct SDLAFormat {
		SDL_AudioFormat format;

		SDLAFormat(SDL_AudioFormat fmt);
		SDLAFormat(int issigned, int isbig, int isfloat, int bitsize);
		SDLAFormat(const AFormat& afmt);
		bool isSigned() const;
		bool isBigEndian() const;
		bool isFloat() const;
		size_t getBitSize() const;
	};
	struct SDLAFormatCF : SDLAFormat {
		int channels,
			freq;
		SDLAFormatCF() = default;
		SDLAFormatCF(SDLAFormat fmt, int fr);
		SDLAFormatCF(const AFormatF& af);
	};

	struct AFormat {
		enum class Format {
			Mono8,
			Mono16,
			Stereo8,
			Stereo16,
			Invalid
		};
		Format	format;

		AFormat(Format fmt=Format::Invalid);
		AFormat(SDLAFormat f, bool bStereo);
		AFormat(bool b16Bit, bool bStereo);
		int getBitNum() const;
		int getChannels() const;
		size_t getBlockSize() const;
	};
	struct AFormatF : AFormat {
		int		freq;

		AFormatF() = default;
		AFormatF(AFormat fmt, int fr);
	};
	using spn::ByteBuff;
	struct RawData {
		AFormatF	format;
		ByteBuff	buff;

		RawData() = default;
		RawData(size_t sz);
		RawData(RawData&& d);
	};
	constexpr static int MAX_AUDIO_BLOCKNUM = 4;
	extern ov_callbacks OVCallbacksNF, OVCallbacks;
	// VorbisFile wrapper
	class VorbisFile {
		HLRW			_hlRW;
		OggVorbis_File	_ovf;
		AFormatF		_format;
		double			_dTotal;
		int64_t			_iTotal;

		public:
			static size_t ReadOGC(void* ptr, size_t blocksize, size_t nmblock, void* datasource);
			static int SeekOGC(void* datasource, ogg_int64_t offset, int whence);
			static int CloseOGC(void* datasource);
			static long TellOGC(void* datasource);
			static ov_callbacks OVCallbacksNF,
			OVCallbacks;

			VorbisFile(HRW hRW);
			~VorbisFile();
			const AFormatF& getFormat() const;
			//! 一括読み出し
			static RawData ReadAll(HRW hRW);
			//! 指定サイズのデータを読み出し
			size_t read(void* dst, size_t toRead);
			bool isEOF() const;

			bool timeSeek(double s);
			void timeSeekPage(double s);
			bool timeSeekLap(double s);
			void timeSeekPageLap(double s);
			bool pcmSeek(int64_t pos);
			void pcmSeekPage(int64_t pos);
			bool pcmSeekLap(int64_t pos);
			void pcmSeekPageLap(int64_t pos);
			double timeLength() const;
			int64_t pcmLength() const;

			double timeTell() const;
			int64_t pcmTell() const;
	};
}
