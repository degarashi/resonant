#include "font_ft_dep.hpp"
#include "spinner/dir.hpp"
#include <fstream>

namespace rs {
	// ---------------------- FontFamily ----------------------
	FontFamily::Item::Item(int fIdx, HRW hRW): faceIndex(fIdx), hlRW(hRW) {}
	FontFamily::Item::Item(int fIdx, const std::string& p): faceIndex(fIdx), path(p) {}
	FontFamily::Item::Item(Item&& it): faceIndex(it.faceIndex), hlRW(std::move(it.hlRW)), path(std::move(it.path)) {}
	HLFT FontFamily::Item::makeFont() const {
		if(hlRW)
			return mgr_font.newFace(hlRW, faceIndex);
		return mgr_font.newFace(mgr_rw.fromFile(path.get(), RWops::Read, true), faceIndex);
	}
	void FontFamily::loadFamilyWildCard(spn::To8Str pattern) {
		size_t len = pattern.getLength();
		if(len == 0)
			return;

		spn::Dir::PathReset pr;
		spn::Dir::EnumEntryWildCard(pattern.moveTo(), [this](const spn::Dir& dir) {
			loadFamily(mgr_rw.fromFile(dir.plain_utf8(), RWops::Read, true));
		});
	}
	void FontFamily::loadFamily(HRW hRW) {
		HLFT hlf = newFace(hRW, 0);
		auto& face = hlf.ref();
		int nf = face.getNFace();
		_fontMap.emplace(face.getFamilyName(), Item(0, hRW));
		for(int i=1 ; i<nf ; i++) {
			HLFT hlf2 = newFace(hRW, i);
			_fontMap.emplace(hlf2.ref().getFamilyName(), Item(i, hRW));
		}
	}
	HLFT FontFamily::fontFromFamilyName(const std::string& name) const {
		auto itr = _fontMap.find(name);
		if(itr == _fontMap.end())
			return HLFT();
		return itr->second.makeFont();
	}
	HLFT FontFamily::fontFromFile(const std::string& path) {
		return HLFT();
	}
	HLFT FontFamily::fontFromID(CCoreID id) const {
		return HLFT();
	}

	// ---------------------- Font_FTDep ----------------------
	// TODO: 縁取り対応
	Font_FTDep::Font_FTDep(const std::string& name, CCoreID cid):
		_charType(cid.at<CCoreID::CharFlag>()), _boldP(cid.at<CCoreID::Weight>()), _bItalic(cid.at<CCoreID::Italic>())
	{
		if(name.empty())
			_hlFT = mgr_font.fontFromID(cid);
		else
			_hlFT = mgr_font.fontFromFamilyName(name);
		Assert(Trap, _hlFT.valid());

		// TODO: この値は埋め込むのではなくてディスプレイから取得するべき
		constexpr int DPI_W = 300,
					DPI_H = 300;
		int w = cid.at<CCoreID::Width>(),
			h = cid.at<CCoreID::Height>();
		auto& ft = _hlFT.ref();
		switch(cid.at<CCoreID::SizeType>()) {
			case CCoreID::SizeType_Pixel:
				ft.setPixelSizes(w, h); break;
			case CCoreID::SizeType_Point:
				ft.setCharSize(w, h, DPI_W, DPI_H); break;
			case CCoreID::SizeType_LineHeight:
				ft.setSizeFromLine(h); break;
			default:
                AssertP(Trap, false, "invalid sizetype number");
		}
	}
	Font_FTDep::Font_FTDep(Font_FTDep&& d):
		_coreID(d._coreID), _hlFT(std::move(d._hlFT)), _buff(std::move(d._buff)), _charType(d._charType) {}
	Font_FTDep& Font_FTDep::operator = (Font_FTDep&& d) {
		this->~Font_FTDep();
		new(this) Font_FTDep(std::move(d));
		return *this;
	}

	int Font_FTDep::height() const {
		return _hlFT.cref().getFaceInfo().height;
	}
	int Font_FTDep::width(char32_t c) {
		auto& ft = _hlFT.ref();
		ft.prepareGlyph(c, (_charType & CCoreID::CharFlag_AA) ? FTFace::RenderMode::Normal : FTFace::RenderMode::Mono,
							_boldP>0, _bItalic);
		return ft.getGlyphInfo().advanceX;
	}
	int Font_FTDep::maxWidth() const {
		return _hlFT.cref().getFaceInfo().maxWidth;
	}
	CCoreID Font_FTDep::adjustParams(CCoreID cid) { return cid; }
	std::pair<spn::ByteBuff, spn::Rect> Font_FTDep::getChara(char32_t code) {
		auto& ft = _hlFT.ref();
		ft.prepareGlyph(code, (_charType & CCoreID::CharFlag_AA) ? FTFace::RenderMode::Normal : FTFace::RenderMode::Mono,
						_boldP>0, _bItalic);
		const auto& gi = ft.getGlyphInfo();
		spn::ByteBuff buff;
		spn::Rect rect(gi.horiBearingX, gi.horiBearingX + gi.width,
						-gi.horiBearingY, -gi.horiBearingY + gi.height);
		if(gi.nlevel == 2)
			buff = Convert1Bit_8Bit(gi.data, gi.width, gi.pitch, gi.height);
		else {
			// nlevelが2より大きい時は256として扱う
			size_t sz = gi.width * gi.height;
			buff.resize(sz);
			if(gi.pitch == gi.width)
				std::memcpy(&buff[0], gi.data, sz);
			else {
				// pitchを詰める
				const uint8_t* src = gi.data;
				uint8_t* dst = &buff[0];
				for(int i=0 ; i<gi.height ; i++) {
					for(int j=0 ; j<gi.width ; j++)
						std::memcpy(dst, src, gi.width);
					src += gi.pitch;
					dst += gi.width;
				}
			}
		}
		return std::make_pair(std::move(buff), rect);
	}
	spn::ByteBuff Convert1Bit_8Bit(const void* src, int width, int pitch, int nrow) {
		auto* pSrc = reinterpret_cast<const uint8_t*>(src);
		spn::ByteBuff buff(width * nrow);
		auto* dst = &buff[0];
		for(int i=0 ; i<nrow ; i++) {
			for(int j=0 ; j<width ; j++)
				*dst++ = ((int32_t(0) - (pSrc[j/8] & (1<<(7-(j%8)))))>>8) & 0xff;
			pSrc += pitch;
		}
		return std::move(buff);
	}
	namespace {
		template <int NB>
		spn::ByteBuff ExpandBits(const void* src, int width, int pitch, int nrow) {
			auto* pSrc = reinterpret_cast<const uint8_t*>(src);
			spn::ByteBuff buff(width*nrow*NB);
			auto* dst = reinterpret_cast<uint8_t*>(&buff[0]);
			for(int i=0 ; i<nrow ; i++) {
				for(int j=0 ; j<width ; j++) {
					auto tmp = pSrc[j];
					for(int k=0 ; k<NB ; k++)
						*dst++ = tmp;
				}
				pSrc += pitch;
			}
			return std::move(buff);
		}
	}
	spn::ByteBuff Convert8Bit_Packed24Bit(const void* src, int width, int pitch, int nrow) {
		return ExpandBits<3>(src, width, pitch, nrow);
	}
	spn::ByteBuff Convert8Bit_Packed32Bit(const void* src, int width, int pitch, int nrow) {
		return ExpandBits<4>(src, width, pitch, nrow);
	}
}
