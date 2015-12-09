#pragma once
#include "twophaseblur.hpp"

class BilateralBlur : public rs::ObjectT<BilateralBlur, TwoPhaseBlur> {
	private:
		static const rs::IdValue U_BCoeff,
								U_GWeight;
		static const rs::IdValue T_BiLH,
								T_BiLV;
		#define SEQ_BILATERAL \
			((BDispersion)(float)) \
			((BCoeff)(float)(BDispersion)) \
			((GDispersion)(float)) \
			((GWeight)(spn::Vec4)(GDispersion))
		RFLAG_S(BilateralBlur, SEQ_BILATERAL)

	public:
		RFLAG_GETMETHOD_S(SEQ_BILATERAL)
		RFLAG_SETMETHOD_S(SEQ_BILATERAL)
		RFLAG_REFMETHOD_S(SEQ_BILATERAL)
		#undef SEQ_BILATERAL

		BilateralBlur(rs::Priority p);
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(BilateralBlur)
