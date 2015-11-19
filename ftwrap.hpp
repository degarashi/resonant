#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H
#include "spinner/misc.hpp"
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "sdlwrap.hpp"
#include "spinner/error.hpp"
#include "handle.hpp"

#define FTEC_Base(flag, act, ...)	::spn::EChk_code##flag(AAct_##act<std::runtime_error, const char*>("FreeTypeCheck"), ::rs::FTError(), SOURCEPOS, __VA_ARGS__)
#define FTEC(...)					FTEC_Base(_a, __VA_ARGS__)
#define FTEC_D(...)					FTEC_Base(_d, __VA_ARGS__)

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
		private:
			FT_Face		_face;
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
			//! 文字のビットマップを準備
			void prepareGlyph(char32_t code, RenderMode mode, bool bBold, bool bItalic);
			const Info& getGlyphInfo() const;
			const FInfo& getFaceInfo() const;
			const char* getFamilyName() const;
			const char* getStyleName() const;
			int getNFace() const;
			int getFaceIndex() const;
	};
	//! 1bitのモノクロビットマップを8bitのグレースケールへ変換
	spn::ByteBuff Convert1Bit_8Bit(const void* src, int width, int pitch, int nrow);
	//! 8bitのグレースケールを24bitのPackedBitmapへ変換
	spn::ByteBuff Convert8Bit_Packed24Bit(const void* src, int width, int pitch, int nrow);
	//! 8bitのグレースケールを32bitのPackedBitmapへ変換
	spn::ByteBuff Convert8Bit_Packed32Bit(const void* src, int width, int pitch, int nrow);
	#define mgr_ft (::rs::FTLibrary::_ref())
	class FTLibrary : public spn::ResMgrA<FTFace, FTLibrary> {
		FT_Library	_lib;
		public:
			FTLibrary();
			~FTLibrary();
			//! メモリまたはファイルシステム上のフォントファイルから読み込む
			LHdl newFace(rs::HRW hRW, int index);
	};
}
