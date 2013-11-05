#include "ftwrap.hpp"

namespace rs {
	namespace {
		#undef __FTERRORS_H__
		#define FT_ERRORDEF(e, v, s) {e, s},
		#define FT_ERROR_START_LIST {
		#define FT_ERROR_END_LIST {0,0} };
		const std::pair<int, const char*> c_ftErrors[] =
		#include FT_ERRORS_H
	}
	// ---------------------- FTError ----------------------
	const char* FTError::ErrorDesc(int result) {
		if(result != 0) {
			for(auto& e : c_ftErrors) {
				if(e.first == result)
					return e.second;
			}
			return "unknown error";
		}
		return nullptr;
	}
	const char* FTError::GetAPIName() {
		return "FreeType";
	}
	// ---------------------- FTLibrary ----------------------
	FTLibrary::FTLibrary() {
		FTEC(Trap, FT_Init_FreeType, &_lib);
	}
	FTLibrary::~FTLibrary() {
		if(_lib)
			FTEC_P(Trap, FT_Done_FreeType, _lib);
	}
	HLFT FTLibrary::newFace(HRW hRW, int index) {
		FT_Face face;
		HLRW hlRW;
		// 中身がメモリじゃなければ一旦コピーする
		if(!hRW.ref().isMemory())
			hlRW = mgr_rw.fromVector(hRW.ref().readAll());
		else
			hlRW = hRW;

		auto m = hlRW.ref().getMemoryPtrC();
		FTEC(Trap, FT_New_Memory_Face, _lib, reinterpret_cast<const uint8_t*>(m.first), m.second, index, &face);
		return acquire(FTFace(face, hlRW.get()));
	}

	// ---------------------- FTFace ----------------------
	FTFace::FTFace(FT_Face face, HRW hRW): _face(face), _style(Style::Normal), _hlRW(hRW) {}
	FTFace::FTFace(FTFace&& f): _face(f._face), _style(f._style), _hlRW(std::move(f._hlRW)), _finfo(f._finfo), _info(f._info) {
		f._face = nullptr;
	}
	FTFace::~FTFace() {
		if(_face)
			FTEC_P(Trap, FT_Done_Face, _face);
	}
	// met.width>>6 == bitmap.width
	// met.height>>6 == bitmap.height
	// met.horiBearingX>>6 == bitmap_left
	// met.horiBearingY>>6 == bitmap_top
	// 	assert(_info.width == _info.bmp_width &&
	// 			_info.height == _info.bmp_height &&
	// 		  _info.horiBearingX == _info.bmp_left &&
	// 		  _info.horiBearingY == _info.bmp_top);
	void FTFace::prepareGlyph(char32_t code, RenderMode mode) {
		uint32_t gindex = FT_Get_Char_Index(_face, code);
		FTEC(Trap, FT_Load_Glyph, _face, gindex, _style==Style::Normal ? FT_LOAD_DEFAULT : FT_LOAD_NO_BITMAP);
		auto* slot = _face->glyph;
		if(_style == Style::Normal) {
			if(slot->format != FT_GLYPH_FORMAT_BITMAP)
				FT_Render_Glyph(slot, static_cast<FT_Render_Mode>(mode));
		} else if(_style == Style::Bold) {
			int strength = 1 << 6;
			FT_Outline_Embolden(&slot->outline, strength);
			FT_Render_Glyph(slot, static_cast<FT_Render_Mode>(mode));
		} else if(_style == Style::Italic) {
			FT_Matrix mat;
			mat.xx = 1 << 16;
			mat.xy = 0x5800;
			mat.yx = 0;
			mat.yy = 1 << 16;
			FT_Outline_Transform(&slot->outline, &mat);
		}
		Assert(Trap, slot->format == FT_GLYPH_FORMAT_BITMAP);

		auto& met = slot->metrics;
		auto& bm = slot->bitmap;
		_info.data = bm.buffer;
		_info.advanceX = slot->advance.x >> 6;
		_info.nlevel = bm.num_grays;
		_info.pitch = bm.pitch;
		_info.height = bm.rows;
		_info.width = bm.width;
		_info.horiBearingX = met.horiBearingX>>6;
		_info.horiBearingY = met.horiBearingY>>6;
	}
	const FTFace::Info& FTFace::getGlyphInfo() const {
		return _info;
	}
	const FTFace::FInfo& FTFace::getFaceInfo() const {
		return _finfo;
	}
	void FTFace::setPixelSizes(int w, int h) {
		FTEC(Trap, FT_Set_Pixel_Sizes, _face, w, h);
		_updateFaceInfo();
	}
	void FTFace::setCharSize(int w, int h, int dpW, int dpH) {
		FTEC(Trap, FT_Set_Char_Size, _face, w, h, dpW, dpH);
		_updateFaceInfo();
	}
	void FTFace::setSizeFromLine(int lineHeight) {
		FT_Size_RequestRec req;
		req.height = lineHeight;
		req.width = 0;
		req.type = FT_SIZE_REQUEST_TYPE_CELL;
		req.horiResolution = 300;
		req.vertResolution = 300;
		FTEC(Trap, FT_Request_Size, _face, &req);
		_updateFaceInfo();
	}
	void FTFace::_updateFaceInfo() {
		_finfo.maxWidth = _face->max_advance_width;
		_finfo.height = _face->height;
	}
	const char* FTFace::getFamilyName() const { return _face->family_name; }
	const char* FTFace::getStyleName() const { return _face->style_name; }
	int FTFace::getNFace() const { return _face->num_faces; }
	int FTFace::getFaceIndex() const { return _face->face_index; }
}
