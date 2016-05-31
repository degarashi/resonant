#include "gaussblur.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue GaussBlur::U_GaussWeight = rs::IEffect::GlxId::GenUnifId("weight");
const rs::IdValue GaussBlur::T_GaussH = rs::IEffect::GlxId::GenTechId("PostEffect", "GaussH"),
				GaussBlur::T_GaussV = rs::IEffect::GlxId::GenTechId("PostEffect", "GaussV");

spn::RFlagRet GaussBlur::_refresh(CoeffArray_t& dst, CoeffArray*) const {
	const float d = getDispersion();
	float total = 0;
	for(size_t i=0 ; i<countof(dst.value) ; i++) {
		dst.value[i] = std::exp<float>(-0.5f * float(i*i)/d).real();
		if(i==0)
			total += dst.value[i];
		else
			total += 2.f * dst.value[i];
	}
	for(auto& c : dst.value)
		c /= total;
	return {true, 0};
}
void GaussBlur::onDraw(rs::IEffect& e) const {
	auto& a = getCoeffArray();
	TwoPhaseBlur::draw(e, T_GaussH, T_GaussV, [&a](rs::IEffect& e){
		e.setUniform(U_GaussWeight, static_cast<const float*>(a.value), countof(a.value), false);
	});
}
