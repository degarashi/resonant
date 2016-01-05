#include "offsetadd.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue OffsetAdd::T_Offset[] = {
	rs::IEffect::GlxId::GenTechId("PostEffect", "OffsetAdd"),
	rs::IEffect::GlxId::GenTechId("PostEffect", "OffsetAdd2"),
	rs::IEffect::GlxId::GenTechId("PostEffect", "OffsetAdd3")
};
const rs::IdValue OffsetAdd::U_AddTex = rs::IEffect::GlxId::GenUnifId("u_texAdd");
const rs::IdValue OffsetAdd::U_Ratio = rs::IEffect::GlxId::GenUnifId("u_ratio");
const rs::IdValue OffsetAdd::U_Offset = rs::IEffect::GlxId::GenUnifId("u_offset");

OffsetAdd::OffsetAdd(rs::Priority p) {
	_nTex = 0;
	_alpha = 0.5f;
	_offset = 0;
	_ratio = 1;
	setDrawPriority(p);
}
void OffsetAdd::setAdd1(rs::HTex h0) {
	_hlAdd[0] = h0;
	_nTex = 1;
}
void OffsetAdd::setAdd2(rs::HTex h0, rs::HTex h1) {
	_hlAdd[0] = h0;
	_hlAdd[1] = h1;
	_nTex = 2;
}
void OffsetAdd::setAdd3(rs::HTex h0, rs::HTex h1, rs::HTex h2) {
	_hlAdd[0] = h0;
	_hlAdd[1] = h1;
	_hlAdd[2] = h2;
	_nTex = 3;
}
void OffsetAdd::setAlpha(float a) {
	_alpha = a;
}
void OffsetAdd::setOffset(float ofs) {
	_offset = ofs;
}
void OffsetAdd::setRatio(float r) {
	_ratio = r;
}
void OffsetAdd::onDraw(rs::IEffect& e) const {
	if( _nTex < 1)
		return;

	e.setTechPassId(T_Offset[_nTex-1]);
	e.setUniform(U_AddTex, _hlAdd, _nTex, false);
	e.setUniform(rs::unif::Alpha, _alpha);
	e.setUniform(U_Ratio, _ratio);
	e.setUniform(U_Offset, _offset);
	_rect.draw(e);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, OffsetAdd, OffsetAdd, "DrawableObj", NOTHING,
	(setAdd1)(setAdd2)(setAdd3)(setAlpha)(setOffset)(setRatio),
	(rs::Priority))
