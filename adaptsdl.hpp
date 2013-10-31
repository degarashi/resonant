#pragma once
#include "spinner/adaptstream.hpp"
#include "sdlwrap.hpp"

namespace rs {
	struct AdaptSDL : spn::AdaptStream {
		HLRW 	_hlRW;
		const static RWops::Hence c_flag[];

		AdaptSDL(HRW hRW);
		AdaptSDL& read(void* dst, streamsize len) override;
		AdaptSDL& seekg(streamoff dist, Dir dir) override;
		streampos tellg() const override;
	};
	struct AdaptOSDL : spn::AdaptOStream {
		HLRW	_hlRW_o;

		AdaptOSDL(HRW hRW);
		AdaptOSDL& write(const void* src, streamsize len) override;
		AdaptOSDL& seekp(streamoff dist, Dir dir) override;
		streampos tellp() const override;
	};
	struct AdaptIOSDL : AdaptSDL, AdaptOSDL {
		AdaptIOSDL(HRW hRW);
	};
}
