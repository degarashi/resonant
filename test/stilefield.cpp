#include "stile.hpp"
#include "test.hpp"
#include "../glresource.hpp"
#include "geometry.hpp"

STileField::STileField(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
							const float scale, const float height, const float height_att, const float th, const float mv)
	:base_t(rd, n, vn, scale, height, height_att, th, mv)
{
	const int tw = _tileWidth;
	_tile.resize(tw*tw);
	for(int i=0 ; i<tw ; i++) {
		for(int j=0 ; j<tw ; j++) {
			_tile[i*tw+j] = spn::construct(_heightL, j*vn, i*vn, vn, n+1);
		}
	}
	_indexArray = Displacement::MakeIndexArray(vn);
	boom::Vec3V pos;
	boom::IndexV idx;
	boom::geo3d::Geometry::MakeSphere(pos, idx, 32,16);
	const int nV = pos.size();
	std::vector<vertex::collision> vtx(nV);
	for(int i=0 ; i<nV ; i++) {
		vtx[i].pos = pos[i];
		vtx[i].color = spn::Vec4(1,0,0,1);
	}
	_vbSphere = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_vbSphere->get()->initData(std::move(vtx));
	_ibSphere = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
	_ibSphere->get()->initData(std::move(idx));
	setViewDistanceCoeff(0.2f, 1.f);

	setStateNew<St_Default>();
}
void STileField::setViewDistanceCoeff(float dMin, float dCoeff) {
	_viewMin = dMin;
	_viewCoeff = dCoeff;
}
namespace {
	const rs::IdValue T_STile = rs::IEffect::GlxId::GenTechId("Tile", "Shader"),
					T_STileView = rs::IEffect::GlxId::GenTechId("Tile", "ShaderView"),
					U_InterpolateLevel = rs::IEffect::GlxId::GenUnifId("u_interpolateLevel"),
					U_ViewCenter = rs::IEffect::GlxId::GenUnifId("u_viewCenter");
}
#include "engine.hpp"
struct STileField::St_Default : StateT<St_Default> {
	void onDraw(const STileField& self, rs::IEffect& e) const override {
		e.setTechPassId(T_STile);
		e.setVDecl(rs::DrawDecl<vdecl::stile>::GetVDecl());
		e.setUniform(U_ViewCenter, self._center);
		self.outputParams(e);

		self._prepareValues(e);
		const int s = self._tileWidth;
		for(int i=0 ; i<s ; i++) {
			for(int j=0 ; j<s ; j++) {
				e.setUniform(U_Offset, spn::Vec3(j, 0, -i));
				auto lv = self._calcLevel(j+0.5f, i+0.5f);
				e.setUniform(U_InterpolateLevel, float(lv.first));
				self._tile[i*s+j]->draw(e, self._indexArray[lv.first], lv.second);
			}
		}

		e.setTechPassId(T_STileView);
		e.setVDecl(rs::DrawDecl<vdecl::collision>::GetVDecl());
		e.setVStream(self._vbSphere, 0);
		e.setIStream(self._ibSphere);
		auto& en = static_cast<Engine&>(e);
		auto fnDraw = [&self, this, &en](float s){
			spn::AMat44 m = spn::AMat44::Scaling(s,s,s,1);
			spn::Vec3 tmp = self._center;
			tmp.y = 0;
			m *= spn::AMat44::Translation(tmp);
			en.ref3D().setWorld(m);
			en.drawIndexed(GL_TRIANGLES, self._ibSphere->get()->getNElem(), 0);
		};

		const float sq2 = std::sqrt(2.f);
		const float dd = self._viewCoeff * sq2*self._width*2;
		for(int i=0 ; i<self._nLevel-1 ; i++) {
			fnDraw(dd*i + self._viewMin*self._width);
		}
	}
};
std::pair<int,spn::Vec2> STileField::_calcLevel(float x, float y) const {
	if(_nLevel == 1)
		return {0, {-1, 0}};

	x *= _width;
	y *= -_width;
	const float sq2 = std::sqrt(2.f)*_width;
	const float mv = sq2/2;
	const spn::Vec3 center(_center.x, 0, _center.z);
	const float d = std::max(0.f, center.distance(spn::Vec3(x,0,y))-mv);
	const float dd = _viewCoeff * sq2 * 2;
	if(d <= _viewMin*_width)
		return {0, {1e8f, 1e9f}};
	const int div = std::min(_nLevel-1, static_cast<int>(std::floor((d-_viewMin*_width) / dd)));
	return {div, {dd*div+_viewMin*_width, dd*(div+1)+_viewMin*_width}};
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, STileField, STileField, "TileFieldBase", NOTHING,
		(setRayleighCoeff)(setMieCoeff)(setLightDir)(setLightColor)(setLightPower)(setDivide)
		(setViewDistanceCoeff),
		(spn::MTRandom&)(int)(int)(float)(float)(float)(float)(float))
