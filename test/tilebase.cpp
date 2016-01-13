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
	setViewDistanceCoeff(0.1f, 1.f);
	setTextureRepeat(1.f);
	setStateNew<St_Default>();
}
void TileFieldBase::_prepareValues(rs::IEffect& e) const {
	if(_hlTex)
		e.setUniform(rs::unif::texture::Diffuse, _hlTex);
	e.setUniform(U_Repeat, _repeat);
	e.setUniform(U_Scale, _scale);
}
int TileFieldBase::_calcLevel(float x, float y) const {
	x *= _width;
	y *= -_width;
	float d = _center.distance(spn::Vec3(x,0,y));
	if(d <= _dMin)
		return 0;
	if(_nLevel<=2 || d>=_dMax)
		return _nLevel-1;
	d = (d-_dMin) / (_dMax-_dMin);
	return (d * (_nLevel-2))+1;
}
void TileFieldBase::setViewPos(const spn::Vec3& p) {
	_center = p;
}
void TileFieldBase::setViewDistanceCoeff(float dMin, float dMax) {
	_dMin = dMin * _width * _tileWidth;
	_dMax = dMax * _width * _tileWidth;
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
		(setViewDistanceCoeff)
)
