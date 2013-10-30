#pragma once
#include "spinner/adaptstream.hpp"
#include "sdlwrap.hpp"

namespace sdlw {
	struct AdaptSDL : spn::AdaptStream {
		HLRW 	_hlRW;
		const static RWops::Hence c_flag[];

		AdaptSDL(HRW hRW);
		AdaptSDL& read(void* dst, streamsize len) override;
		AdaptSDL& seekg(streamoff dist, Dir dir) override;
		streampos tellg() const override;
	};
}
