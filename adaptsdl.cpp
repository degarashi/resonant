#include "adaptsdl.hpp"

namespace sdlw {
	// ---------------------- AdaptSDL ----------------------
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
	// ---------------------- AdaptOSDL ----------------------
	AdaptOSDL::AdaptOSDL(HRW hRW): _hlRW_o(hRW) {}
	AdaptOSDL& AdaptOSDL::write(const void* src, streamsize len) {
		_hlRW_o.ref().write(src, len, 1);
		return *this;
	}
	AdaptOSDL& AdaptOSDL::seekp(streamoff dist, Dir dir) {
		_hlRW_o.ref().seek(dist, AdaptSDL::c_flag[dir]);
		return *this;
	}
	AdaptSDL::streampos AdaptOSDL::tellp() const {
		return _hlRW_o.cref().tell();
	}
	// ---------------------- AdaptIOSDL ----------------------
	AdaptIOSDL::AdaptIOSDL(HRW hRW): AdaptSDL(hRW), AdaptOSDL(hRW) {};
}
