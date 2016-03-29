#include "twophaseblur.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue U_MapWidth = rs::IEffect::GlxId::GenUnifId("mapWidth");
TwoPhaseBlur::TwoPhaseBlur(rs::Priority p) {
	setDrawPriority(p);
}
spn::RFlagRet TwoPhaseBlur::_refresh(Tmp_t& dst, Tmp*) const {
	auto *t0 = getSource()->get(),
		*t1 = getDest()->get();
	auto sz0 = t0->getSize();
	auto sz1 = t1->getSize();
	Assert(Trap, sz0==sz1, "Source texture size is not equals to destination texture size")
	auto fmt = getTmpFormat();
	if(!fmt) {
		auto fmt0 = *t0->getFormat();
		auto fmt1 = *t1->getFormat();
		Assert(Trap, fmt0==fmt1, "Source texture size is not equals to destination texture size")
		fmt = fmt0;
	}
	dst.tmp = mgr_gl.createTexture(sz0, fmt->get(), false, false);
	dst.tmp->get()->setLinear(true);
	_hlFb = mgr_gl.makeFBuffer();
	dst.rb = mgr_gl.makeRBuffer(sz0.width, sz0.height, GL_DEPTH_COMPONENT16);
	_hlFb->get()->attachRBuffer(rs::GLFBuffer::Att::DEPTH, dst.rb);
	return {true, 0};
}
void TwoPhaseBlur::draw(rs::IEffect& e, rs::IdValue tp0, rs::IdValue tp1, const CBDraw& cbDraw) const {
	rs::HLFb fb = e.getFramebuffer();

	auto& src = getSource();
	auto& dst = getDest();
	auto& tmp = getTmp();
	const auto sz = src->get()->getSize();
	const spn::Vec2 szv(sz.width, sz.height);

	// ---- Phase 1 ----
	e.setTechPassId(tp0);
	e.setUniform(U_MapWidth, szv);
	e.setUniform(rs::unif::texture::Diffuse, src);
	cbDraw(e);
	_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, tmp.tmp);
	e.setFramebuffer(_hlFb);

	rs::draw::ClearParam cp;
	cp.color = spn::Vec4(0.5f);
	cp.depth = 1.f;
	e.clearFramebuffer(cp);

	_rect.draw(e);

	// ---- Phase 2 ----
	e.setTechPassId(tp1);
	e.setUniform(U_MapWidth, szv);
	e.setUniform(rs::unif::texture::Diffuse, tmp.tmp);
	cbDraw(e);
	_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, dst);
	e.setFramebuffer(_hlFb);
	_rect.draw(e);

	e.setFramebuffer(fb);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TwoPhaseBlur, TwoPhaseBlur, "Object", NOTHING,
	(setSource<rs::HTex>)
	(setDest<rs::HTex>)
	(setTmpFormat<int>),
	(rs::Priority))
