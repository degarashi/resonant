#include "sdlwrap.hpp"

namespace rs {
	// -------------------- Surface --------------------
	Surface::Surface(SDL_Surface* sfc): _sfc(sfc) {}
	Surface::~Surface() {
		SDL_FreeSurface(_sfc);
	}
	SPSurface Surface::Load(HRW hRW) {
		SDL_RWops* rw = hRW.ref().getOps();
		SDL_Surface* sfc = IMGEC(Trap, IMG_Load_RW, rw, 0);
		return SPSurface(new Surface(sfc));
	}
	void Surface::saveAsBMP(HRW hDst) const {
		SDLEC(Trap, SDL_SaveBMP_RW, _sfc, hDst.ref().getOps(), 0);
	}
	void Surface::saveAsPNG(HRW hDst) const {
		IMGEC(Trap, IMG_SavePNG_RW, _sfc, hDst.ref().getOps(), 0);
	}
	void* Surface::lock() {
		_mutex.lock();
		SDLEC(Trap, SDL_LockSurface, _sfc);
		return _sfc->pixels;
	}
	void* Surface::try_lock() {
		if(_mutex.try_lock()) {
			SDLEC(Trap, SDL_LockSurface, _sfc);
			return _sfc->pixels;
		}
		return nullptr;
	}
	void Surface::unlock() {
		SDLEC(Trap, SDL_UnlockSurface, _sfc);
		_mutex.unlock();
	}
	spn::Size Surface::getSize() const {
		return spn::Size(_sfc->w, _sfc->h);
	}
	const SDL_PixelFormat& Surface::getFormat() const {
		return *_sfc->format;
	}
}
