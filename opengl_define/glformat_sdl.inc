// 変換元SDLフォーマット, 変換先SDLフォーマット(OpenGL互換フォーマット)
// 一旦右側のフォーマットに変換してからs_SDLtoGL.find(format)とする
const static std::pair<uint32_t, GLenum> c_SDLtoGL[] = {
#ifdef USE_OPENGLES2
	// ES2に存在しないので互換フォーマットを選択
	{SDL_PIXELFORMAT_RGB332, GL_RGB},
	{SDL_PIXELFORMAT_BGR555, GL_RGB},
	{SDL_PIXELFORMAT_RGB555, GL_RGB},
	{SDL_PIXELFORMAT_RGB444, GL_RGB},
	{SDL_PIXELFORMAT_ABGR8888, GL_RGBA},
	{SDL_PIXELFORMAT_ARGB8888, GL_RGBA},
	{SDL_PIXELFORMAT_BGRA8888, GL_RGBA},
	{SDL_PIXELFORMAT_RGBA8888, GL_RGBA},
	{SDL_PIXELFORMAT_BGR888, GL_RGB},
	{SDL_PIXELFORMAT_BGRX8888, GL_RGB},
	{SDL_PIXELFORMAT_RGB888, GL_RGB},
	{SDL_PIXELFORMAT_RGBX8888, GL_RGB},
	{SDL_PIXELFORMAT_BGR24, GL_RGB},
	{SDL_PIXELFORMAT_RGB24, GL_RGB},
	{SDL_PIXELFORMAT_ARGB2101010, GL_RGBA},
	// ES2にあってGL4に無いフォーマット
	{SDL_PIXELFORMAT_BGR565, GL_RGB565},
	{SDL_PIXELFORMAT_RGB565, GL_RGB565},
#else
	// GL4にだけ存在するフォーマット
	{SDL_PIXELFORMAT_RGB332, GL_R3_G3_B2},
	{SDL_PIXELFORMAT_BGR555, GL_RGB5},
	{SDL_PIXELFORMAT_RGB555, GL_RGB5},
	{SDL_PIXELFORMAT_RGB444, GL_RGB4},
	{SDL_PIXELFORMAT_ABGR8888, GL_RGBA8},
	{SDL_PIXELFORMAT_ARGB8888, GL_RGBA8},
	{SDL_PIXELFORMAT_BGRA8888, GL_RGBA8},
	{SDL_PIXELFORMAT_RGBA8888, GL_RGBA8},
	{SDL_PIXELFORMAT_BGR888, GL_RGB8},
	{SDL_PIXELFORMAT_BGRX8888, GL_RGB8},
	{SDL_PIXELFORMAT_RGB888, GL_RGB8},
	{SDL_PIXELFORMAT_RGBX8888, GL_RGB8},
	{SDL_PIXELFORMAT_BGR24, GL_RGB8},
	{SDL_PIXELFORMAT_RGB24, GL_RGB8},
	{SDL_PIXELFORMAT_ARGB2101010, GL_RGB10_A2},
	// GL4にあってES2に無いフォーマット
	{SDL_PIXELFORMAT_BGR565, GL_RGB8},
	{SDL_PIXELFORMAT_RGB565, GL_RGB8},
#endif
	// 両方に存在するフォーマット
	{SDL_PIXELFORMAT_ABGR1555, GL_RGB5_A1},
	{SDL_PIXELFORMAT_ARGB1555, GL_RGB5_A1},
	{SDL_PIXELFORMAT_BGRA5551, GL_RGB5_A1},
	{SDL_PIXELFORMAT_RGBA5551, GL_RGB5_A1},
	{SDL_PIXELFORMAT_ABGR4444, GL_RGBA4},
	{SDL_PIXELFORMAT_ARGB4444, GL_RGBA4},
	{SDL_PIXELFORMAT_BGRA4444, GL_RGBA4},
	{SDL_PIXELFORMAT_RGBA4444, GL_RGBA4}
};

// 現在のところSDL2はR、RGや64bit以上のピクセルフォーマットに対応していないようなのでこれらの値は無効化
#define SDL_PIXELFORMAT_R8				SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RG88			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGB101010		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_BGR101010		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGBA1010102		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_ABGR2101010		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_R16				SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RG1616			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGB161616		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_BGR161616		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGBA16161616	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_ABGR16161616	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_R32				SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RG3232			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGB323232		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_BGR323232		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGBA32323232	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_ABGR32323232	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_R16F			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RG1616F			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGB161616F		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_BGR161616F		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGBA16161616F	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_ABGR16161616F	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_R32F			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RG3232F			SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGB323232F		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_BGR323232F		SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_RGBA32323232F	SDL_PIXELFORMAT_UNKNOWN
#define SDL_PIXELFORMAT_ABGR32323232F	SDL_PIXELFORMAT_UNKNOWN

// (GLフォーマット型, 保存する時のType, 1画素に使うType数, BaseFormatにする場合の型, 対応するSDLの型(ない場合はUNKNOWN))
const static GLFormatDesc c_GLFormatList[] = {
	{GL_RGB, GL_UNSIGNED_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA, GL_UNSIGNED_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_ABGR8888},
	{GL_DEPTH_COMPONENT16, GL_UNSIGNED_SHORT, 1, GL_DEPTH_COMPONENT,
		SDL_PIXELFORMAT_R16, SDL_PIXELFORMAT_RGB24},
	{GL_DEPTH_COMPONENT, GL_FLOAT, 1, GL_DEPTH_COMPONENT,
		SDL_PIXELFORMAT_R32F, SDL_PIXELFORMAT_RGB24},
	{GL_STENCIL_INDEX8, GL_UNSIGNED_BYTE, 1, GL_STENCIL_INDEX,
		SDL_PIXELFORMAT_R8, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA4, GL_UNSIGNED_SHORT_4_4_4_4, 1, GL_RGBA,
		SDL_PIXELFORMAT_ABGR4444, SDL_PIXELFORMAT_ABGR4444},
	{GL_RGB5_A1, GL_UNSIGNED_SHORT_5_5_5_1, 1, GL_RGBA,
		SDL_PIXELFORMAT_ABGR1555, SDL_PIXELFORMAT_ABGR1555},
#ifdef USE_OPENGLES2
	{GL_ALPHA, GL_UNSIGNED_BYTE, 1, GL_ALPHA,
		SDL_PIXELFORMAT_R8, SDL_PIXELFORMAT_RGB24},
	{GL_LUMINANCE, GL_FLOAT, 1, GL_LUMINANCE,
		SDL_PIXELFORMAT_R32F, SDL_PIXELFORMAT_RGB24},
	{GL_LUMINANCE_ALPHA, GL_FLOAT, 1, GL_LUMINANCE_ALPHA,
		SDL_PIXELFORMAT_R32F, SDL_PIXELFORMAT_RGB24},
	{GL_RGB565, GL_UNSIGNED_SHORT_5_6_5, 1, GL_RGB,
		SDL_PIXELFORMAT_BGR565, SDL_PIXELFORMAT_BGR565},
#else
	{GL_RED, GL_UNSIGNED_BYTE, 1, GL_RED,
		SDL_PIXELFORMAT_R8, SDL_PIXELFORMAT_RGB24},
	{GL_RG, GL_UNSIGNED_BYTE, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_BGR, GL_UNSIGNED_BYTE, 3, GL_BGR,
		SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_BGR24},
	{GL_BGRA, GL_UNSIGNED_BYTE, 4, GL_BGRA,
		SDL_PIXELFORMAT_ARGB8888, SDL_PIXELFORMAT_ARGB8888},
	{GL_DEPTH_STENCIL, GL_FLOAT, 2, GL_DEPTH_STENCIL,
		SDL_PIXELFORMAT_RG3232F, SDL_PIXELFORMAT_RGB24},
	{GL_R8, GL_UNSIGNED_BYTE, 1, GL_RED,
		SDL_PIXELFORMAT_R8, SDL_PIXELFORMAT_RGB24},
	{GL_R8_SNORM, GL_BYTE, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R16, GL_UNSIGNED_SHORT, 1, GL_RED,
		SDL_PIXELFORMAT_R16, SDL_PIXELFORMAT_RGB24},
	{GL_R16_SNORM, GL_SHORT, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG8, GL_UNSIGNED_BYTE, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG8_SNORM, GL_BYTE, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG16, GL_UNSIGNED_SHORT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG16_SNORM, GL_SHORT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R3_G3_B2, GL_UNSIGNED_BYTE_3_3_2, 1, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB4, GL_UNSIGNED_SHORT_4_4_4_4, 1, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR4444},
	{GL_RGB5, GL_UNSIGNED_SHORT_5_5_5_1, 1, GL_RGB,
		SDL_PIXELFORMAT_BGR555, SDL_PIXELFORMAT_BGR555},
	{GL_RGB8, GL_UNSIGNED_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB24},
	{GL_RGB8_SNORM, GL_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB10, GL_UNSIGNED_INT_10_10_10_2, 1, GL_RGB,
		SDL_PIXELFORMAT_BGR101010, SDL_PIXELFORMAT_RGB24},
	{GL_RGB12, GL_UNSIGNED_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB16_SNORM, GL_SHORT, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA2, GL_UNSIGNED_SHORT_4_4_4_4, 1, GL_RGBA,
		SDL_PIXELFORMAT_ABGR4444, SDL_PIXELFORMAT_ABGR4444},
	{GL_RGBA8, GL_UNSIGNED_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA8_SNORM, GL_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGB10_A2, GL_UNSIGNED_INT_10_10_10_2, 1, GL_RGBA,
		SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_ARGB2101010},
	{GL_RGB10_A2UI, GL_UNSIGNED_INT_10_10_10_2, 1, GL_RGBA,
		SDL_PIXELFORMAT_ABGR2101010, SDL_PIXELFORMAT_ARGB2101010},
	{GL_RGBA12, GL_UNSIGNED_SHORT, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA16, GL_UNSIGNED_SHORT, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR16161616, SDL_PIXELFORMAT_ABGR8888},
	{GL_SRGB8, GL_UNSIGNED_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_SRGB8_ALPHA8, GL_UNSIGNED_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_R16F, GL_HALF_FLOAT, 1, GL_RED,
		SDL_PIXELFORMAT_R16F, SDL_PIXELFORMAT_RGB24},
	{GL_RG16F, GL_HALF_FLOAT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB16F, GL_HALF_FLOAT, 3, GL_RGB,
		SDL_PIXELFORMAT_BGR161616F, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA16F, GL_HALF_FLOAT, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR16161616F, SDL_PIXELFORMAT_ABGR8888},
	{GL_R32F, GL_FLOAT, 1, GL_RED,
		SDL_PIXELFORMAT_R32F, SDL_PIXELFORMAT_RGB24},
	{GL_RG32F, GL_FLOAT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB32F, GL_FLOAT, 3, GL_RGB,
		SDL_PIXELFORMAT_BGR323232F, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA32F, GL_FLOAT, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR32323232F, SDL_PIXELFORMAT_ABGR8888},
	{GL_R11F_G11F_B10F, GL_HALF_FLOAT, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB9_E5, GL_UNSIGNED_INT_5_9_9_9_REV, 1, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_R8I, GL_BYTE, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R8UI, GL_UNSIGNED_BYTE, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R16I, GL_SHORT, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R16UI, GL_UNSIGNED_SHORT, 1, GL_RED,
		SDL_PIXELFORMAT_R16, SDL_PIXELFORMAT_RGB24},
	{GL_R32I, GL_INT, 1, GL_RED,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_R32UI, GL_UNSIGNED_INT, 1, GL_RED,
		SDL_PIXELFORMAT_R32, SDL_PIXELFORMAT_RGB24},
	{GL_RG8I, GL_BYTE, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG8UI, GL_UNSIGNED_BYTE, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG16I, GL_SHORT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG16UI, GL_UNSIGNED_SHORT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG32I, GL_INT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RG32UI, GL_UNSIGNED_INT, 2, GL_RG,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB8I, GL_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB8UI, GL_UNSIGNED_BYTE, 3, GL_RGB,
		SDL_PIXELFORMAT_RGB24, SDL_PIXELFORMAT_RGB24},
	{GL_RGB16I, GL_SHORT, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB16UI, GL_UNSIGNED_SHORT, 3, GL_RGB,
		SDL_PIXELFORMAT_BGR161616, SDL_PIXELFORMAT_RGB24},
	{GL_RGB32I, GL_INT, 3, GL_RGB,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24},
	{GL_RGB32UI, GL_UNSIGNED_INT, 3, GL_RGB,
		SDL_PIXELFORMAT_BGR323232, SDL_PIXELFORMAT_RGB24},
	{GL_RGBA8I, GL_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA8UI, GL_UNSIGNED_BYTE, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR8888, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA16I, GL_SHORT, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA16UI, GL_UNSIGNED_SHORT, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR16161616, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA32I, GL_INT, 4, GL_RGBA,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_ABGR8888},
	{GL_RGBA32UI, GL_UNSIGNED_INT, 4, GL_RGBA,
		SDL_PIXELFORMAT_ABGR32323232, SDL_PIXELFORMAT_ABGR8888},
	{GL_DEPTH_COMPONENT24, GL_UNSIGNED_INT, 1, GL_DEPTH_COMPONENT,
		SDL_PIXELFORMAT_R32, SDL_PIXELFORMAT_RGB24},
	{GL_DEPTH_COMPONENT32, GL_UNSIGNED_INT, 1, GL_DEPTH_COMPONENT,
		SDL_PIXELFORMAT_R32, SDL_PIXELFORMAT_RGB24},
	{GL_DEPTH_COMPONENT32F, GL_FLOAT, 1, GL_DEPTH_COMPONENT,
		SDL_PIXELFORMAT_R32F, SDL_PIXELFORMAT_RGB24},
	{GL_STENCIL_INDEX4, GL_UNSIGNED_BYTE, 1, GL_STENCIL_INDEX,
		SDL_PIXELFORMAT_R8, SDL_PIXELFORMAT_RGB24},
	{GL_STENCIL_INDEX16, GL_UNSIGNED_SHORT, 1, GL_STENCIL_INDEX,
		SDL_PIXELFORMAT_R16, SDL_PIXELFORMAT_RGB24},
	{GL_DEPTH24_STENCIL8, GL_FLOAT_32_UNSIGNED_INT_24_8_REV, 1, GL_DEPTH_STENCIL,
		SDL_PIXELFORMAT_UNKNOWN, SDL_PIXELFORMAT_RGB24}
#endif
};
