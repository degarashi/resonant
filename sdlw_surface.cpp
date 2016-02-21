#include "sdlwrap.hpp"

namespace rs {
	// ------------------------ RGB ------------------------
	RGB::RGB(int r, int g, int b):
		ar{uint8_t(r), uint8_t(g), uint8_t(b)} {}
	// ------------------------ RGBA ------------------------
	RGBA::RGBA(RGB rgb, int a): ar{rgb.r, rgb.g, rgb.b, static_cast<uint8_t>(a)} {}
	RGBA::RGBA(const std::initializer_list<int>& il) {
		AssertP(Trap, il.size()==4)
		auto* pDst = ar;
		for(auto& v : il)
			*pDst++ = static_cast<uint8_t>(v);
	}

	UPSDLFormat MakeUPFormat(uint32_t fmt) {
		return std::unique_ptr<SDL_PixelFormat, decltype(&SDL_FreeFormat)>(SDL_AllocFormat(fmt), SDL_FreeFormat);
	}
	namespace {
		#define DEF_FSTR(name)	{name, #name},
		// フォーマット文字列
		const std::pair<uint32_t, std::string> c_formatName[] = {
			// 非デフォルト定義のフォーマット
			DEF_FSTR(SDL_PIXELFORMAT_R8)
			DEF_FSTR(SDL_PIXELFORMAT_RG88)
			DEF_FSTR(SDL_PIXELFORMAT_RGB101010)
			DEF_FSTR(SDL_PIXELFORMAT_BGR101010)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA1010102)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR2101010)
			DEF_FSTR(SDL_PIXELFORMAT_R16)
			DEF_FSTR(SDL_PIXELFORMAT_RG1616)
			DEF_FSTR(SDL_PIXELFORMAT_RGB161616)
			DEF_FSTR(SDL_PIXELFORMAT_BGR161616)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA16161616)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR16161616)
			DEF_FSTR(SDL_PIXELFORMAT_R32)
			DEF_FSTR(SDL_PIXELFORMAT_RG3232)
			DEF_FSTR(SDL_PIXELFORMAT_RGB323232)
			DEF_FSTR(SDL_PIXELFORMAT_BGR323232)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA32323232)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR32323232)
			DEF_FSTR(SDL_PIXELFORMAT_R16F)
			DEF_FSTR(SDL_PIXELFORMAT_RG1616F)
			DEF_FSTR(SDL_PIXELFORMAT_RGB161616F)
			DEF_FSTR(SDL_PIXELFORMAT_BGR161616F)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA16161616F)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR16161616F)
			DEF_FSTR(SDL_PIXELFORMAT_R32F)
			DEF_FSTR(SDL_PIXELFORMAT_RG3232F)
			DEF_FSTR(SDL_PIXELFORMAT_RGB323232F)
			DEF_FSTR(SDL_PIXELFORMAT_BGR323232F)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA32323232F)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR32323232F)
			// デフォルト定義のフォーマット
			DEF_FSTR(SDL_PIXELFORMAT_RGB332)
			DEF_FSTR(SDL_PIXELFORMAT_BGR555)
			DEF_FSTR(SDL_PIXELFORMAT_RGB555)
			DEF_FSTR(SDL_PIXELFORMAT_RGB444)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR8888)
			DEF_FSTR(SDL_PIXELFORMAT_ARGB8888)
			DEF_FSTR(SDL_PIXELFORMAT_BGRA8888)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA8888)
			DEF_FSTR(SDL_PIXELFORMAT_BGR888)
			DEF_FSTR(SDL_PIXELFORMAT_BGRX8888)
			DEF_FSTR(SDL_PIXELFORMAT_RGB888)
			DEF_FSTR(SDL_PIXELFORMAT_RGBX8888)
			DEF_FSTR(SDL_PIXELFORMAT_BGR24)
			DEF_FSTR(SDL_PIXELFORMAT_RGB24)
			DEF_FSTR(SDL_PIXELFORMAT_ARGB2101010)
			DEF_FSTR(SDL_PIXELFORMAT_BGR565)
			DEF_FSTR(SDL_PIXELFORMAT_RGB565)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR1555)
			DEF_FSTR(SDL_PIXELFORMAT_ARGB1555)
			DEF_FSTR(SDL_PIXELFORMAT_BGRA5551)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA5551)
			DEF_FSTR(SDL_PIXELFORMAT_ABGR4444)
			DEF_FSTR(SDL_PIXELFORMAT_ARGB4444)
			DEF_FSTR(SDL_PIXELFORMAT_BGRA4444)
			DEF_FSTR(SDL_PIXELFORMAT_RGBA4444)
		};
		#undef DEF_FSTR
	}
	uint32_t Surface::Map(uint32_t format, RGB rgb) {
		return Map(format, RGBA(rgb, 255));
	}
	uint32_t Surface::Map(uint32_t format, RGBA rgba) {
		auto fmt = MakeUPFormat(format);
		return SDLEC_D(Trap, SDL_MapRGBA, fmt.get(), rgba.r, rgba.g, rgba.b, rgba.a);
	}
	RGBA Surface::Get(uint32_t format, uint32_t pixel) {
		auto fmt = MakeUPFormat(format);
		RGBA ret;
		SDLEC_D(Trap, SDL_GetRGBA, pixel, fmt.get(), &ret.r, &ret.g, &ret.b, &ret.a);
		return ret;
	}
	const std::string& Surface::GetFormatString(uint32_t format) {
		for(auto& p : c_formatName) {
			if(p.first == format)
				return p.second;
		}
		const static std::string c_unknown("SDL_PIXELFORMAT_UNKNOWN");
		return c_unknown;
	}

	// -------------------- Surface --------------------
	Surface::Surface(SDL_Surface* sfc): _sfc(sfc), _buff(nullptr, 0) {}
	Surface::Surface(SDL_Surface* sfc, spn::ByteBuff&& buff): _sfc(sfc), _buff(std::move(buff)) {}
	Surface::~Surface() {
		SDL_FreeSurface(_sfc);
	}
	SPSurface Surface::Create(int w, int h, uint32_t format) {
		auto fmt = MakeUPFormat(format);
		auto* sfc = SDLEC(Trap, SDL_CreateRGBSurface, 0, w, h, fmt->BitsPerPixel, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
		return SPSurface(new Surface(sfc));
	}
	SPSurface Surface::Create(const spn::ByteBuff& src, int pitch, int w, int h, uint32_t format) {
		return Create(spn::ByteBuff(src), pitch, w, h, format);
	}
	SPSurface Surface::Create(spn::ByteBuff&& src, int pitch, int w, int h, uint32_t format) {
		auto fmt = MakeUPFormat(format);
		if(pitch==0)
			pitch = fmt->BytesPerPixel*w;
		auto* sfc = SDLEC(Trap, SDL_CreateRGBSurfaceFrom, &src[0], w, h, fmt->BitsPerPixel, pitch, fmt->Rmask, fmt->Gmask, fmt->Bmask, fmt->Amask);
		return SPSurface(new Surface(sfc, std::move(src)));
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
		SDLEC_D(Trap, SDL_FillRect, _sfc, &r, color);
	}
	Surface::LockObj Surface::lock() const {
		_mutex.lock();
		SDLEC_D(Trap, SDL_LockSurface, _sfc);
		return LockObj(*this, _sfc->pixels, _sfc->pitch);
	}
	Surface::LockObj Surface::try_lock() const {
		if(_mutex.try_lock()) {
			SDLEC_D(Trap, SDL_LockSurface, _sfc);
			return LockObj(*this, _sfc->pixels, _sfc->pitch);
		}
		return LockObj(*this, nullptr, 0);
	}
	void Surface::_unlock() const {
		SDLEC_D(Trap, SDL_UnlockSurface, _sfc);
		_mutex.unlock();
	}
	spn::Size Surface::getSize() const {
		return spn::Size(_sfc->w, _sfc->h);
	}
	const SDL_PixelFormat& Surface::getFormat() const {
		return *_sfc->format;
	}

	Surface::LockObj::LockObj(const Surface& sfc, void* bits, int pitch): _sfc(sfc), _bits(bits), _pitch(pitch) {}
	Surface::LockObj::LockObj(LockObj&& lk): _sfc(lk._sfc), _bits(lk._bits), _pitch(lk._pitch) {}
	Surface::LockObj::~LockObj() {
		if(_bits)
			_sfc._unlock();
	}
	Surface::LockObj::operator bool() const {
		return _bits != nullptr;
	}
	void* Surface::LockObj::getBits() {
		return _sfc._sfc->pixels;
	}
	int Surface::LockObj::getPitch() const {
		return _sfc._sfc->pitch;
	}
	int Surface::width() const {
		return _sfc->w;
	}
	int Surface::height() const {
		return _sfc->h;
	}
	SPSurface Surface::convert(uint32_t fmt) const {
		SDL_Surface* nsfc = SDL_ConvertSurfaceFormat(_sfc, fmt, 0);
		Assert(Trap, nsfc)
		return SPSurface(new Surface(nsfc));
	}
	SPSurface Surface::convert(const SDL_PixelFormat& fmt) const {
		SDL_Surface* nsfc = SDL_ConvertSurface(_sfc, const_cast<SDL_PixelFormat*>(&fmt), 0);
		Assert(Trap, nsfc)
		return SPSurface(new Surface(nsfc));
	}
	uint32_t Surface::getFormatEnum() const {
		return _sfc->format->format;
	}
	bool Surface::isContinuous() const {
		return _sfc->pitch == _sfc->w * _sfc->format->BytesPerPixel;
	}
	spn::ByteBuff Surface::extractAsContinuous(uint32_t dstFmt) const {
		auto& myformat = getFormat();
		if(dstFmt == 0)
			dstFmt = myformat.format;

		auto lk = lock();
		int w = width(),
			h = height();
		// ピクセルデータが隙間なく詰まっていて、なおかつフォーマットも同じならそのままメモリをコピー
		if(isContinuous() && dstFmt==myformat.format) {
			auto* src = reinterpret_cast<uint8_t*>(lk.getBits());
			return spn::ByteBuff(src, src + w*h*myformat.BytesPerPixel);
		}
		auto upFmt = MakeUPFormat(dstFmt);
		size_t dstSize = w * h * upFmt->BytesPerPixel;
		spn::ByteBuff dst(dstSize);
		SDLEC(Trap, SDL_ConvertPixels,
					w,h,
					myformat.format, lk.getBits(), lk.getPitch(),
					dstFmt, &dst[0], w*upFmt->BytesPerPixel);
		return dst;
	}
	SDL_Surface* Surface::getSurface() {
		return _sfc;
	}
	namespace {
		SDL_Rect ToSDLRect(const spn::Rect& r) {
			SDL_Rect ret;
			ret.x = r.x0;
			ret.y = r.y0;
			ret.w = r.width();
			ret.h = r.height();
			return ret;
		}
	}
	void Surface::blit(const SPSurface& sfc, const spn::Rect& srcRect, int dstX, int dstY) const {
		SDL_Rect sr = ToSDLRect(srcRect),
				dr;
		dr.x = dstX;
		dr.y = dstY;
		dr.w = dr.w;
		dr.h = dr.h;
		SDLEC(Trap, SDL_BlitSurface, _sfc, &sr, sfc->getSurface(), &dr);
	}
	void Surface::blitScaled(const SPSurface& sfc, const spn::Rect& srcRect, const spn::Rect& dstRect) const {
		SDL_Rect sr = ToSDLRect(srcRect),
				dr = ToSDLRect(dstRect);
		SDLEC(Trap, SDL_BlitScaled, _sfc, &sr, sfc->getSurface(), &dr);
	}
	SPSurface Surface::resize(const spn::Size& s) const {
		auto bm = getBlendMode();
		auto* self = const_cast<Surface*>(this);
		self->setBlendMode(SDL_BLENDMODE_NONE);

		auto sz = getSize();
		spn::Rect srcRect(0,sz.width, 0,sz.height),
				dstRect(0,s.width, 0,s.height);
		SPSurface nsfc = Create(s.width, s.height, getFormat().format);
		blitScaled(nsfc, srcRect, dstRect);

		self->setBlendMode(bm);
		return nsfc;
	}
	SPSurface Surface::makeBlank() const {
		return Create(width(), height(), getFormatEnum());
	}
	SPSurface Surface::duplicate() const {
		auto sp = makeBlank();
		blit(sp, {0,width(),0,height()}, 0, 0);
		return sp;
	}
	SPSurface Surface::flipHorizontal() const {
		auto sp = makeBlank();
		int w = width(),
			h = height();
		for(int i=0 ; i<w ; i++)
			blit(sp, {i,i+1,0,h}, w-i-1, 0);
		return sp;
	}
	SPSurface Surface::flipVertical() const {
		auto sp = makeBlank();
		int w = width(),
			h = height();
		for(int i=0 ; i<h ; i++)
			blit(sp, {0,w,i,i+1}, 0, h-i-1);
		return sp;
	}
	void Surface::setEnableColorKey(uint32_t key) {
		SDLEC(Trap, SDL_SetColorKey, _sfc, SDL_TRUE, key);
	}
	void Surface::setDisableColorKey() {
		SDLEC(Trap, SDL_SetColorKey, _sfc, SDL_FALSE, 0);
	}
	spn::Optional<uint32_t> Surface::getColorKey() const {
		uint32_t key;
		if(SDLEC(Trap, SDL_GetColorKey, _sfc, &key) == -1)
			return spn::none;
		return key;
	}
	void Surface::setBlendMode(SDL_BlendMode mode) {
		SDLEC(Trap, SDL_SetSurfaceBlendMode, _sfc, mode);
	}
	SDL_BlendMode Surface::getBlendMode() const {
		SDL_BlendMode mode;
		int ret = SDLEC(Trap, SDL_GetSurfaceBlendMode, _sfc, &mode);
		Assert(Warn, ret >= 0, "unknown error")
		return mode;
	}
}
