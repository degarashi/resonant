#include "sdlwrap.hpp"

namespace rs {
	// -------------------- Color --------------------
	const Color::Info Color::c_pfInfo[] = {
		{SDL_PIXELFORMAT_RGB888, 24, 0x00ff0000, 0x0000ff00, 0x000000ff},
		{SDL_PIXELFORMAT_RGBA8888, 32, 0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff},
		{SDL_PIXELFORMAT_ARGB8888, 32, 0x000000ff, 0xff000000, 0x00ff0000, 0x0000ff00},
		{SDL_PIXELFORMAT_RGB555, 16, 0x1e00, 0x01e0, 0x001f, 0x0000},
		{SDL_PIXELFORMAT_RGBA5551, 16, 0xf800, 0x07c0, 0x003e, 0x0001},
		{SDL_PIXELFORMAT_ARGB1555, 16, 0x7c00, 0x03e0, 0x001f, 0x8000}
	};
	uint32_t Color::Map(PFormat format, int r, int g, int b) {
		return Map(format, r, g, b, 255);
	}
	uint32_t Color::Map(PFormat format, int r, int g, int b, int a) {
		auto& info = c_pfInfo[format];
		SDL_PixelFormat fmt = {};
		fmt.format = info.sdlformat;
		fmt.BitsPerPixel = info.bitsize;
		fmt.BytesPerPixel = (info.bitsize+7)/8;
		fmt.Rmask = info.maskR;
		fmt.Gmask = info.maskG;
		fmt.Bmask = info.maskB;
		fmt.Amask = info.maskA;
		return SDLEC_P(Trap, SDL_MapRGBA, &fmt,
			static_cast<uint8_t>(r),
			static_cast<uint8_t>(g),
			static_cast<uint8_t>(b),
			static_cast<uint8_t>(a));
	}

	// -------------------- Surface --------------------
	Surface::Surface(SDL_Surface* sfc): _sfc(sfc) {}
	Surface::~Surface() {
		SDL_FreeSurface(_sfc);
	}
	SPSurface Surface::Create(int w, int h, Color::PFormat format) {
		auto& info = Color::c_pfInfo[format];
		auto* sfc = SDLEC(Trap, SDL_CreateRGBSurface, 0, w, h, info.bitsize, info.maskR, info.maskG, info.maskB, info.maskA);
		return SPSurface(new Surface(sfc));
	}
	SPSurface Surface::Create(void* pSrc, int pitch, int w, int h, Color::PFormat format) {
		auto& info = Color::c_pfInfo[format];
		auto* sfc = SDLEC(Trap, SDL_CreateRGBSurfaceFrom, pSrc, w, h, info.bitsize, pitch, info.maskR, info.maskG, info.maskB, info.maskA);
		return SPSurface(new Surface(sfc));
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
	void Surface::fillRect(const spn::Rect& rect, uint32_t color) {
		SDL_Rect r;
		r.x = rect.x0;
		r.y = rect.y0;
		r.w = rect.width();
		r.h = rect.height();
		SDLEC_P(Trap, SDL_FillRect, _sfc, &r, color);
	}
	void* Surface::lock() {
		_mutex.lock();
		SDLEC_P(Trap, SDL_LockSurface, _sfc);
		return _sfc->pixels;
	}
	void* Surface::try_lock() {
		if(_mutex.try_lock()) {
			SDLEC_P(Trap, SDL_LockSurface, _sfc);
			return _sfc->pixels;
		}
		return nullptr;
	}
	void Surface::unlock() {
		SDLEC_P(Trap, SDL_UnlockSurface, _sfc);
		_mutex.unlock();
	}
	spn::Size Surface::getSize() const {
		return spn::Size(_sfc->w, _sfc->h);
	}
	const SDL_PixelFormat& Surface::getFormat() const {
		return *_sfc->format;
	}
}
