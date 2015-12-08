#pragma once
#include "../util/screen.hpp"
#include "spinner/rflag.hpp"

extern const rs::IdValue U_MapWidth,
						U_Weight;
extern const rs::IdValue T_GaussH,
						T_GaussV;
class GaussBlur : public rs::DrawableObjT<GaussBlur> {
	private:
		struct CoeffArray_t {
			float value[8];
		};
		struct Tmp_t {
			rs::HLTex	tmp;
			rs::HLRb	rb;
		};
		#define SEQ_GAUSS \
			((Source)(rs::HLTex)) \
			((Dest)(rs::HLTex)) \
			((Tmp)(Tmp_t)(Source)(Dest)) \
			((Dispersion)(float)) \
			((CoeffArray)(CoeffArray_t)(Dispersion))
		RFLAG_S(GaussBlur, SEQ_GAUSS)
		mutable rs::HLFb		_hlFb;
		rs::util::ScreenRect	_rect;

	public:
		RFLAG_GETMETHOD_S(SEQ_GAUSS)
		RFLAG_SETMETHOD_S(SEQ_GAUSS)
		RFLAG_REFMETHOD_S(SEQ_GAUSS)
		#undef SEQ_GAUSS

		GaussBlur(rs::Priority p);
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(GaussBlur)
