#include "skydome.hpp"
#include "engine.hpp"
#include "../camera.hpp"
#include "diffusion_u.hpp"

namespace {
	using GId = rs::IEffect::GlxId;
	const rs::IdValue T_Skydome =	GId::GenTechId("Tile", "Skydome");
}

SkyDome::SkyDome(rs::Priority p): PostEffect(T_Skydome, p) {
	setScale(1, 1);
	setDivide(2e1f);
	setRayleighCoeff({680*1e-3f, 550*1e-3f, 480*1e-3f});
	setMieCoeff(0.9f, 0.1f);
	setLightPower(24.4f);
	setLightColor({1,1,1});
	setLightDir({0,-1,0});
}
void SkyDome::setScale(const float w, const float h) {
	setParam(U_SdScale, spn::Vec2{w,h});
}
void SkyDome::setDivide(const float d) {
	setParam(U_SdDivide, d);
}
void SkyDome::setRayleighCoeff(const spn::Vec3& r) {
	setParam(U_Rayleigh, r);
}
void SkyDome::setMieCoeff(const float gain, const float c) {
	setParam(U_Mie, spn::Vec2{gain, c});
}
void SkyDome::setLightPower(const float p) {
	setParam(U_LPower, p);
}
void SkyDome::setLightDir(const spn::Vec3& dir) {
	setParam(U_LDir, dir);
}
void SkyDome::setLightColor(const spn::Vec3& c) {
	setParam(U_LColor, c);
}
void SkyDome::onDraw(rs::IEffect& e) const {
	auto& en = static_cast<Engine&>(e);
	en.ref3D().getCamera()->getView();
	PostEffect::onDraw(e);
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SkyDome, SkyDome, "DrawableObj", NOTHING,
	(setScale)
	(setDivide)
	(setRayleighCoeff)
	(setMieCoeff)
	(setLightPower)
	(setLightDir)
	(setLightColor),
	(rs::Priority)
)
