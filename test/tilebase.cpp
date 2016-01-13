#include "tilebase.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

const rs::IdValue U_Repeat = rs::IEffect::GlxId::GenUnifId("u_repeat"),
				U_Scale = rs::IEffect::GlxId::GenUnifId("u_scale"),
				U_Offset = rs::IEffect::GlxId::GenUnifId("u_tileoffset");
struct TileFieldBase::St_Default : StateT<St_Default> {};

TileFieldBase::TileFieldBase(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
					const float scale, const float height, const float height_att, const float th, const float mv)
{
	_heightL = Displacement::MakeDisplacement(rd, n, height_att);
	Displacement::Smooth(_heightL, n, th, mv);
	_index = Displacement::MakeBuffer(Displacement::MakeIndex(vn));
	_center = spn::Vec3(0);
	const int tw = n / vn;
	_width = scale / tw;
	_tileWidth = tw;
	_nLevel = _index.size();
	_scale = {_width, height, _width};
	setTextureRepeat(1.f);
	setStateNew<St_Default>();
}
void TileFieldBase::_prepareValues(rs::IEffect& e) const {
	if(_hlTex)
		e.setUniform(rs::unif::texture::Diffuse, _hlTex);
	e.setUniform(U_Repeat, _repeat);
	e.setUniform(U_Scale, _scale);
}
void TileFieldBase::setViewPos(const spn::Vec3& p) {
	_center = p;
}
void TileFieldBase::setTexture(rs::HTex hTex) {
	_hlTex = hTex;
	_hlTex->get()->setUVWrap(rs::WrapState::MirroredRepeat, rs::WrapState::MirroredRepeat);
}
void TileFieldBase::setTextureRepeat(float r) {
	_repeat = r;
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, TileFieldBase, TileFieldBase, "DrawableObj", NOTHING,
		(setTexture)
		(setTextureRepeat)
		(setViewPos)
)
