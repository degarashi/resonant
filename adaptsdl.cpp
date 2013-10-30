#include "adaptsdl.hpp"

namespace sdlw {
	const RWops::Hence AdaptSDL::c_flag[] = {RWops::Begin, RWops::Current, RWops::End};
	AdaptSDL::AdaptSDL(HRW hRW): _hlRW(hRW) {}
	AdaptSDL& AdaptSDL::read(void* dst, streamsize len) {
		_hlRW.ref().read(dst, len, 1);
		return *this;
	}
	AdaptSDL& AdaptSDL::seekg(streamoff dist, Dir dir) {
		_hlRW.ref().seek(dist, c_flag[dir]);
		return *this;
	}
	AdaptSDL::streampos AdaptSDL::tellg() const {
		return _hlRW.cref().tell();
	}
}
