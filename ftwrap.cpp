#include "ftwrap.hpp"

namespace rs {
	// ---------------------- FTLibrary ----------------------
	FTLibrary::FTLibrary() {
		FT_Init_FreeType(&_lib);
	}
	FTLibrary::~FTLibrary() {
		if(_lib)
			FT_Done_FreeType(_lib);
	}
	HLFT FTLibrary::newFace(HRW hRW, int index) {
		FT_Face face;
		auto buff = hRW.ref().readAll();
		FT_New_Memory_Face(_lib, &buff[0], buff.size(), index, &face);
		return acquire(FTFace(face));
	}

	// ---------------------- FTFace ----------------------
	FTFace::FTFace(FT_Face face): _face(face), _style(Style::Normal) {}
	FTFace::FTFace(FTFace&& f): _face(f._face) {
		f._face = nullptr;
	}
	FTFace::~FTFace() {
		if(_face)
			FT_Done_Face(_face);
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
		FT_Load_Glyph(_face, gindex, _style==Style::Normal ? FT_LOAD_DEFAULT : FT_LOAD_NO_BITMAP);
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
		assert(slot->format == FT_GLYPH_FORMAT_BITMAP);

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
		FT_Set_Pixel_Sizes(_face, w, h);
		_updateFaceInfo();
	}
	void FTFace::setCharSize(int w, int h, int dpW, int dpH) {
		FT_Set_Char_Size(_face, w, h, dpW, dpH);
		_updateFaceInfo();
	}
	void FTFace::setSizeFromLine(int lineHeight) {
		FT_Size_RequestRec req;
		req.height = lineHeight;
		req.width = 0;
		req.type = FT_SIZE_REQUEST_TYPE_CELL;
		req.horiResolution = req.vertResolution = 0;
		FT_Request_Size(_face, &req);
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
