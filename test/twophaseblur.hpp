#pragma once
#include "../util/screen.hpp"
#include "spinner/rflag.hpp"

class TwoPhaseBlur : public rs::DrawableObjT<TwoPhaseBlur> {
	private:
		static const rs::IdValue U_MapWidth;
		struct Tmp_t {
			rs::HLTex	tmp;
			rs::HLRb	rb;
		};
		#define SEQ_TWOPHASE \
			((Source)(rs::HLTex)) \
			((Dest)(rs::HLTex)) \
			((TmpFormat)(rs::OPInSizedFmt)) \
			((Tmp)(Tmp_t)(Source)(Dest)(TmpFormat))
		RFLAG_S(TwoPhaseBlur, SEQ_TWOPHASE)
		mutable rs::HLFb		_hlFb;
		rs::util::ScreenRect	_rect;

	protected:
		using CBDraw = std::function<void (rs::IEffect&)>;
		void draw(rs::IEffect& e, rs::IdValue tp0, rs::IdValue tp1, const CBDraw& cbDraw) const;
	public:
		RFLAG_GETMETHOD_S(SEQ_TWOPHASE)
		RFLAG_SETMETHOD_S(SEQ_TWOPHASE)
		RFLAG_REFMETHOD_S(SEQ_TWOPHASE)
		#undef SEQ_TWOPHASE

		TwoPhaseBlur(rs::Priority p);
};
DEF_LUAIMPORT(TwoPhaseBlur)
