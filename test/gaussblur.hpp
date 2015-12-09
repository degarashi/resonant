#pragma once
#include "twophaseblur.hpp"

class GaussBlur : public rs::ObjectT<GaussBlur, TwoPhaseBlur> {
	private:
		static const rs::IdValue U_GaussWeight;
		static const rs::IdValue T_GaussH,
								T_GaussV;
		struct CoeffArray_t {
			float value[8];
		};
		#define SEQ_GAUSS \
			((Dispersion)(float)) \
			((CoeffArray)(CoeffArray_t)(Dispersion))
		RFLAG_S(GaussBlur, SEQ_GAUSS)

	public:
		RFLAG_GETMETHOD_S(SEQ_GAUSS)
		RFLAG_SETMETHOD_S(SEQ_GAUSS)
		RFLAG_REFMETHOD_S(SEQ_GAUSS)
		#undef SEQ_GAUSS

		using ObjectT::ObjectT;
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(GaussBlur)
