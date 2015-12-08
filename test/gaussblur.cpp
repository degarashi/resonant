#include "gaussblur.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue U_MapWidth = rs::IEffect::GlxId::GenUnifId("mapWidth"),
				U_Weight = rs::IEffect::GlxId::GenUnifId("weight");
const rs::IdValue T_GaussH = rs::IEffect::GlxId::GenTechId("PostEffect", "GaussH"),
				T_GaussV = rs::IEffect::GlxId::GenTechId("PostEffect", "GaussV");
GaussBlur::GaussBlur(rs::Priority p) {
	setDrawPriority(p);
}
spn::RFlagRet GaussBlur::_refresh(Tmp_t& dst, Tmp*) const {
	auto *t0 = getSource()->get(),
		*t1 = getDest()->get();
	auto sz0 = t0->getSize();
	auto sz1 = t1->getSize();
	Assert(Trap, sz0==sz1, "Source texture size is not equals to destination texture size")
	auto fmt0 = *t0->getFormat();
	auto fmt1 = *t1->getFormat();
	Assert(Trap, fmt0==fmt1, "Source texture size is not equals to destination texture size")
	dst.tmp = mgr_gl.createTexture(sz0, fmt0.get(), false, false);
	dst.tmp->get()->setFilter(true, true);
	_hlFb = mgr_gl.makeFBuffer();
	dst.rb = mgr_gl.makeRBuffer(sz0.width, sz0.height, GL_DEPTH_COMPONENT16);
	_hlFb->get()->attachRBuffer(rs::GLFBuffer::Att::DEPTH, dst.rb);
	return {true, 0};
}
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
	auto& src = getSource();
	auto& dst = getDest();
	auto& tmp = getTmp();
	const auto sz = src->get()->getSize();
	const spn::Vec2 szv(sz.width, sz.height);
	auto& a = getCoeffArray();

	// ---- Gauss H ----
	e.setTechPassId(T_GaussH);
	e.setUniform(U_MapWidth, szv);
	e.setUniform(rs::unif::texture::Diffuse, src);
	auto wid = *e.getUnifId(U_Weight);
	e.setUniform(wid, static_cast<const float*>(a.value), countof(a.value), false);
	_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, tmp.tmp);
	e.setFramebuffer(_hlFb);
	_rect.draw(e);

	// ---- Gauss V ----
	e.setTechPassId(T_GaussV);
	e.setUniform(U_MapWidth, szv);
	e.setUniform(rs::unif::texture::Diffuse, tmp.tmp);
	e.setUniform(wid, static_cast<const float*>(a.value), countof(a.value));
	_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, dst);
	e.setFramebuffer(_hlFb);
	_rect.draw(e);
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, GaussBlur, GaussBlur, "Object", NOTHING,
	(setSource<rs::HTex>)
	(setDest<rs::HTex>)
	(setDispersion<float>),
	(rs::Priority))
