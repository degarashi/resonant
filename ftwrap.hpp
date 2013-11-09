#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "spinner/misc.hpp"
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "sdlwrap.hpp"
#include "error.hpp"

#define FTEC(act, func, ...)	::rs::EChk_baseA2(AAct_##act<std::runtime_error>(), ::rs::FTError(), __FILE__, __PRETTY_FUNCTION__, __LINE__, func(__VA_ARGS__))
#ifdef DEBUG
	#define FTEC_P(act, ...)	FTEC(act, __VA_ARGS__)
#else
	#define FTEC_P(act, ...)	::rs::EChk_pass(__VA_ARGS__)
#endif

namespace rs {
	struct FTError {
		const char* errorDesc(int result) const;
		const char* getAPIName() const;
	};
	class FTFace {
		public:
			enum class RenderMode {
				Normal = FT_RENDER_MODE_NORMAL,
				Mono = FT_RENDER_MODE_MONO,
				LCD = FT_RENDER_MODE_LCD
			};
			enum class Style {
				Normal,
				Bold,
				Italic,
				Strike
			};
		private:
			FT_Face		_face;
			Style		_style;
			HLRW		_hlRW;
			struct FInfo {
				int	baseline,
					height,
					maxWidth;		// フォントの最大横幅
			};
			FInfo		_finfo;
			struct Info {
				const uint8_t* data;
				int advanceX;		// 原点を進める幅
				int nlevel;
				int width,			// bitmapの横
					height,			// bitmapの縦
					pitch;			// bitmapのピッチ(バイト)
				int horiBearingX,	// originから右にずらしたサイズ
					horiBearingY;	// baseLineから上に出るサイズ
			};
			Info		_info;
			void _updateFaceInfo();

		public:
			FTFace(FT_Face face, HRW hRW=HRW());
			FTFace(FTFace&& f);
			~FTFace();
			void setPixelSizes(int w, int h);
			void setCharSize(int w, int h, int dpW, int dpH);
			void setSizeFromLine(int lineHeight);
			void setStyle(Style style);
			//! 文字のビットマップを準備
			void prepareGlyph(char32_t code, RenderMode mode);
			const Info& getGlyphInfo() const;
			const FInfo& getFaceInfo() const;
			const char* getFamilyName() const;
			const char* getStyleName() const;
			int getNFace() const;
			int getFaceIndex() const;
			Style getStyle() const;
	};
	//! 1bitのモノクロビットマップを8bitのグレースケールへ変換
	spn::ByteBuff Convert1Bit_8Bit(const void* src, int width, int pitch, int nrow);
	//! 8bitのグレースケールを24bitのPackedBitmapへ変換
	spn::ByteBuff Convert8Bit_Packed24Bit(const void* src, int width, int pitch, int nrow);
	#define mgr_ft (::rs::FTLibrary::_ref())
	class FTLibrary : public spn::ResMgrA<FTFace, FTLibrary> {
		FT_Library	_lib;
		public:
			FTLibrary();
			~FTLibrary();
			//! メモリまたはファイルシステム上のフォントファイルから読み込む
			LHdl newFace(rs::HRW hRW, int index);
	};
	DEF_HANDLE(FTLibrary, FT, FTFace)
}
