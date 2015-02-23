#include "font_ft_dep.hpp"
#include "spinner/dir.hpp"
#include <fstream>
#include "spinner/emplace.hpp"

namespace rs {
	// ---------------------- FontFamily ----------------------
	FontFamily::Item::Item(int fIdx, HRW hRW): faceIndex(fIdx), hlRW(hRW) {}
	FontFamily::Item::Item(int fIdx, const std::string& p): faceIndex(fIdx), path(p) {}
	HLFT FontFamily::Item::makeFont() const {
		if(hlRW)
			return mgr_font.newFace(hlRW, faceIndex);
		return mgr_font.newFace(mgr_rw.fromFile(path.get(), RWops::Read), faceIndex);
	}
	void FontFamily::loadFamilyWildCard(spn::To8Str pattern) {
		size_t len = pattern.getLength();
		if(len == 0)
			return;

		spn::Dir::EnumEntryWildCard(pattern.moveTo(), [this](const spn::Dir& dir) {
			loadFamily(mgr_rw.fromFile(dir.plain_utf8(), RWops::Read));
		});
	}

	void FontFamily::loadFamily(HRW hRW) {
		HLFT hlf = newFace(hRW, 0);
		auto& face = hlf.ref();
		int nf = face.getNFace();
		spn::EmplaceOrReplace(_fontMap, face.getFamilyName(), 0, hRW);
		for(int i=1 ; i<nf ; i++) {
			HLFT hlf2 = newFace(hRW, i);
			spn::EmplaceOrReplace(_fontMap, hlf2.ref().getFamilyName(), i, hRW);
		}
	}
	HLFT FontFamily::fontFromFamilyName(const std::string& name) const {
		auto itr = _fontMap.find(name);
		if(itr == _fontMap.end())
			return HLFT();
		return itr->second.makeFont();
	}
	HLFT FontFamily::fontFromFile(const std::string& path) {
		Assert(Trap, false, "not implemented yet")
		return HLFT();
	}
	HLFT FontFamily::fontFromID(CCoreID id) const {
		Assert(Trap, false, "not implemented yet")
		return HLFT();
	}

	// ---------------------- Font_FTDep ----------------------
	namespace {
		void SetFTSize(FTFace& ft, CCoreID coreID) {
			// TODO: この値は埋め込むのではなくてディスプレイから取得するべき
			constexpr int DPI_W = 300,
						DPI_H = 300;
			int w = coreID.at<CCoreID::Width>(),
				h = coreID.at<CCoreID::Height>();
			switch(coreID.at<CCoreID::SizeType>()) {
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
		void PrepareGlyph(FTFace& ft, CharID cid) {
			ft.prepareGlyph(cid.code, (cid.at<CCoreID::CharFlag>() & CCoreID::CharFlag_AA) ? FTFace::RenderMode::Normal : FTFace::RenderMode::Mono,
								cid.at<CCoreID::Weight>()>0,
								cid.at<CCoreID::Italic>());
		}
	}
	// TODO: 縁取り対応
	Font_FTDep::Font_FTDep(const std::string& name, CCoreID cid):
		_coreID(cid)
	{
		_hlFT = mgr_font.fontFromFamilyName(name);
		Assert(Trap, _hlFT.valid());
		SetFTSize(_hlFT.ref(), _coreID);
	}
	Font_FTDep::Font_FTDep(CCoreID cid):
		_coreID(cid)
	{
		_hlFT = mgr_font.fontFromID(cid);
		Assert(Trap, _hlFT.valid());
		SetFTSize(_hlFT.ref(), _coreID);
	}

	int Font_FTDep::height() const {
		return _hlFT.cref().getFaceInfo().height;
	}
	int Font_FTDep::width(char32_t c) {
		auto& ft = _hlFT.ref();
		SetFTSize(ft, _coreID);
		PrepareGlyph(ft, CharID(c, _coreID));
		return ft.getGlyphInfo().advanceX;
	}
	int Font_FTDep::maxWidth() const {
		return _hlFT.cref().getFaceInfo().maxWidth;
	}
	CCoreID Font_FTDep::adjustParams(CCoreID cid) { return cid; }
	std::pair<spn::ByteBuff, spn::Rect> Font_FTDep::getChara(char32_t c) {
		auto& ft = _hlFT.ref();
		SetFTSize(ft, _coreID);
		PrepareGlyph(ft, CharID(c, _coreID));
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
