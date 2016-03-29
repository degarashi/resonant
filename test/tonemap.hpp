#pragma once
#include "../updater.hpp"
#include "spinner/rflag.hpp"
#include "../util/screenrect.hpp"

class ToneMap : public rs::DrawableObjT<ToneMap> {
	private:
		using base_t = rs::DrawableObjT<ToneMap>;
		using HLTexV = std::vector<rs::HLTex>;
		#define SEQ_Tone \
			((Source)(rs::HLTex)) \
			((ScreenSizeP)(spn::Size)(Source)) \
			((Shrink0)(rs::HLTex)(ScreenSizeP)) \
			((ShrinkA)(HLTexV)(ScreenSizeP)) \
			((Result)(rs::HLTex)(Source))
		RFLAG_S(ToneMap, SEQ_Tone)

		mutable rs::HLFb		_fbFirst,
								_fbShrink,
								_fbApply;
		rs::util::ScreenRect	_rect;

		struct St_Default;
	public:
		RFLAG_SETMETHOD_S(SEQ_Tone)
		RFLAG_GETMETHOD_S(SEQ_Tone)
		#undef SEQ_Tone

		ToneMap(rs::Priority p);
};
DEF_LUAIMPORT(ToneMap)
