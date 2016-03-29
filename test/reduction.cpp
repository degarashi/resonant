#include "reduction.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

extern const rs::IdValue U_MapWidth;
const rs::IdValue Reduction::T_Reduct2 = rs::IEffect::GlxId::GenTechId("PostEffect", "Reduction2"),
				Reduction::T_Reduct4 = rs::IEffect::GlxId::GenTechId("PostEffect", "Reduction4");
void Reduction::onDraw(rs::IEffect& e) const {
	if(!_hlSource)
		return;
	rs::HLFb fb = e.getFramebuffer();

	auto* src = _hlSource.cref().get();
	auto sz = src->getSize();
	auto szt = sz / _getRatio();
	auto fmt = src->getFormat();
	if(!_hlResult ||
		_hlResult->get()->getSize() != szt ||
		_hlResult->get()->getFormat() != fmt)
	{
		_hlResult = mgr_gl.createTexture(szt, fmt->get(), false, false);
		_hlResult->get()->setLinear(true);
	}
	e.setTechPassId(_getTechId());
	e.setUniform(U_MapWidth, spn::Vec2(sz.width, sz.height));
	e.setUniform(rs::unif::texture::Diffuse, _hlSource);
	_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, _hlResult);
	e.setFramebuffer(_hlFb);
	_rect.draw(e);

	e.setFramebuffer(fb);
}
rs::IdValue Reduction::_getTechId() const {
	return _bFour ? T_Reduct4 : T_Reduct2;
}
int Reduction::_getRatio() const {
	return _bFour ? 4 : 2;
}
void Reduction::setSource(rs::HLTex h) {
	_hlSource = h;
}
rs::HTex Reduction::getResult() const {
	return _hlResult;
}
Reduction::Reduction(rs::Priority p, bool bFour):
	_bFour(bFour)
{
	_hlFb = mgr_gl.makeFBuffer();
	setDrawPriority(p);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, Reduction, Reduction, "Object", NOTHING,
	(setSource)(getResult),
	(rs::Priority)(bool))
