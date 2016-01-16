#include "skydome.hpp"
#include "geometry.hpp"
#include "engine.hpp"
#include "test.hpp"
#include "../camera.hpp"

namespace {
	const rs::IdValue T_Skydome = rs::IEffect::GlxId::GenTechId("Tile", "Skydome");
// 						U_SkyScale =	rs::IEffect::GlxId::GenUnifId("u_skyScale"),
// 						U_RayCoeff =	rs::IEffect::GlxId::GenUnifId("u_raycoeff"),
// 						U_MieCoeff =	rs::IEffect::GlxId::GenUnifId("u_miecoeff"),
// 						U_SunColor =	rs::IEffect::GlxId::GenUnifId("u_sunColor"),
// 						U_SunDir =		rs::IEffect::GlxId::GenUnifId("u_sunDir"),
// 						U_DomeSize =	rs::IEffect::GlxId::GenUnifId("u_domeSize");
}
// struct SkyDome::St_Default : StateT<St_Default> {
// 	void onDraw(const SkyDome& self, rs::IEffect& e) const override {
// 		e.setTechPassId(T_Skydome);
// 		e.setVDecl(rs::DrawDecl<vdecl::skydome>::GetVDecl());
// 		e.setVStream(self._vb, 0);
// 		e.setIStream(self._ib);
// 		e.setUniform(U_SkyScale, spn::Vec2(self._height, self._width));
// 		auto& en = static_cast<Engine&>(e);
// 		auto m = spn::AMat44::RotationX(spn::DegF(90.f));
// // 		m *= spn::AMat44::Translation(en.ref3D().getCamera()->getPose().getOffset());
// 		en.ref3D().setWorld(m);
// 		e.drawIndexed(GL_TRIANGLES, self._ib->get()->getNElem(), 0);
// 	}
// };
// SkyDome::SkyDome(const int nH, const int nV) {
// 	boom::Vec3V pos;
// 	boom::IndexV idx;
// 	boom::geo3d::Geometry::MakeSphere(pos, idx, nH, nV);
// 	const int nVt = pos.size();
// 	std::vector<vertex::skydome> vtx(nVt);
// 	for(int i=0 ; i<nVt ; i++) {
// 		vtx[i].pos = pos[i];
// 	}
//
// 	_vb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
// 	_vb->get()->initData(std::move(vtx));
// 	_ib = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
// 	_ib->get()->initData(std::move(idx));
//
// 	setHeight(1e1f);
// 	setWidth(1e3f);
// 	setStateNew<St_Default>();
// }
// void SkyDome::setHeight(const float h) {
// 	_height = h;
// }
// void SkyDome::setWidth(const float w) {
// 	_width = w;
// }
//
// // ---------------------- SkyDome頂点宣言 ----------------------
// const rs::SPVDecl& rs::DrawDecl<vdecl::skydome>::GetVDecl() {
// 	static rs::SPVDecl vd(new rs::VDecl{
// 		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION}
// 	});
// 	return vd;
// }
// #include "../updater_lua.hpp"
// DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SkyDome, SkyDome, "DrawableObj", NOTHING,
// 		(setHeight)
// 		(setWidth),
// 		(int)(int))

// ---------------------- SkyDome頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::skydome>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::POSITION}
	});
	return vd;
}

SkyDome::SkyDome(rs::Priority p): PostEffect(T_Skydome, p) {
	setRayleighCoeff({680*1e-3f, 550*1e-3f, 480*1e-3f});
	setMieCoeff(0.75f, 1.f);
	setSunColor({1,1,1});
	setSunDir({0,-1,0});
	setSize(1, 500);
}
void SkyDome::onDraw(rs::IEffect& e) const {
	auto& en = static_cast<Engine&>(e);
	en.ref3D().getCamera()->getView();
	PostEffect::onDraw(e);
}
void SkyDome::setRayleighCoeff(const spn::Vec3& c) {
// 	setParam(U_RayCoeff, c);
}
void SkyDome::setMieCoeff(float gain, float c) {
// 	setParam(U_MieCoeff, spn::Vec2{gain, c});
}
void SkyDome::setSunColor(const spn::Vec3& c) {
// 	setParam(U_SunColor, c);
}
void SkyDome::setSunDir(const spn::Vec3& dir) {
// 	setParam(U_SunDir, dir);
}
void SkyDome::setSize(float w, float h) {
// 	setParam(U_DomeSize, spn::Vec2{w,h});
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SkyDome, SkyDome, "DrawableObj", NOTHING,
	(setRayleighCoeff)
	(setMieCoeff)
	(setSunColor)
	(setSunDir)
	(setSize),
	(rs::Priority)
)
