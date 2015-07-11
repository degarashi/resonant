#include "../sys_uniform_value.hpp"
#include "blur.hpp"
#include "test.hpp"

// --------------------- Blur ---------------------
const rs::IdValue T_Blur = GlxId::GenTechId("BlurTech", "P0");
Blur::Blur(rs::Priority dprio):
	_alpha(0.5f)
{
	_dtag.priority = dprio;
}
void Blur::setAlpha(float a) {
	_alpha = a;
}
void Blur::onDraw(rs::GLEffect& e) const {
	e.setTechPassId(T_Blur);
	e.setUniform(rs::unif::texture::Diffuse, _hlTex);
	e.setUniform(rs::unif::Alpha, _alpha);
	// 重ねて描画
	_rect.draw(e);
}
void Blur::setTexture(rs::HTex hTex) {
	_hlTex = hTex;
}

// --------------------- FBSwitch ---------------------
FBSwitch::FBSwitch(rs::Priority dprio, rs::HFb hFb):
	_hlFb(hFb)
{
	_dtag.priority = dprio;
}
void FBSwitch::onDraw(rs::GLEffect& e) const {
	if(_hlFb)
		e.setFramebuffer(_hlFb);
	else
		e.resetFramebuffer();
	// これ自体の描画はしない
}

// --------------------- FBClear ---------------------
FBClear::FBClear(rs::Priority dprio,
				const rs::draw::ClearParam& p):
	_param(p)
{
	_dtag.priority = dprio;
}
void FBClear::onDraw(rs::GLEffect& e) const {
	e.clearFramebuffer(_param);
}
