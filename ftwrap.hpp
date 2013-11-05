#pragma once
#include <ft2build.h>
#include FT_FREETYPE_H
#include "spinner/misc.hpp"
#include "spinner/abstbuff.hpp"
#include "spinner/resmgr.hpp"
#include "sdlwrap.hpp"

namespace rs {
	class FTFace {
		public:
			enum class RenderMode {
				Normal = FT_RENDER_MODE_NORMAL,
				Mono = FT_RENDER_MODE_MONO,
				LCD = FT_RENDER_MODE_LCD
			};
		private:
			FT_Face		_face;
			struct FInfo {
				int	height,
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
			FTFace(FT_Face face);
			FTFace(FTFace&& f);
			~FTFace();
			void setPixelSizes(int w, int h);
			void setCharSize(int w, int h, int dpW, int dpH);
			//! 文字のビットマップを準備
			void prepareGlyph(char32_t code, RenderMode mode);
			const Info& getGlyphInfo() const;
			const FInfo& getFaceInfo() const;
			const char* getFamilyName() const;
			const char* getStyleName() const;
			int getNFace() const;
			int getFaceIndex() const;
	};
	//! 1bitのモノクロビットマップを8bitのグレースケールへ変換
	spn::ByteBuff Convert1Bit_8Bit(const void* src, int width, int pitch, int nrow);
	#define mgr_ft FTLibrary::_ref()
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
