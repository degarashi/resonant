#include "bilateralblur.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue BilateralBlur::U_BCoeff = rs::IEffect::GlxId::GenUnifId("bl_coeff"),
				BilateralBlur::U_GWeight = rs::IEffect::GlxId::GenUnifId("bl_weight");
const rs::IdValue BilateralBlur::T_BiLH = rs::IEffect::GlxId::GenTechId("PostEffect", "BilateralH"),
					BilateralBlur::T_BiLV = rs::IEffect::GlxId::GenTechId("PostEffect", "BilateralV");

BilateralBlur::BilateralBlur(rs::Priority p):
	ObjectT(p)
{
	setBDispersion(1e-2f);
	setGDispersion(0.5f);
	setTmpFormat(GL_RGBA16F);
}
spn::RFlagRet BilateralBlur::_refresh(float& dst, BCoeff*) const {
	auto d = getBDispersion();
	dst = -0.5f / (d*d);
	return {true, 0};
}
spn::RFlagRet BilateralBlur::_refresh(spn::Vec4& dst, GWeight*) const {
	const auto d = getGDispersion();
	for(int i=0 ; i<4 ; i++) {
		float tmp = 2.f * float(i) + 1.5f;
		dst.m[i] = 2.f * std::exp<float>(-0.5f * (tmp*tmp) / d).real();
	}
	return {true, 0};
}
void BilateralBlur::onDraw(rs::IEffect& e) const {
	TwoPhaseBlur::draw(e, T_BiLH, T_BiLV, [c=getBCoeff(), d=getGWeight()](rs::IEffect& e){
		e.setUniform(U_BCoeff, c);
		e.setUniform(U_GWeight, d);
	});
}
