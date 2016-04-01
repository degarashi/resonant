#include "diffusion_u.hpp"

namespace {
	using GId = rs::IEffect::GlxId;
}
const rs::IdValue
		U_SdScale =		GId::GenUnifId("u_sdScale"),
		U_SdDivide =	GId::GenUnifId("u_sdDivide"),
		U_Rayleigh =	GId::GenUnifId("u_rayleigh"),
		U_Mie =			GId::GenUnifId("u_mie"),
		U_LPower =		GId::GenUnifId("u_lPower"),
		U_LDir =		GId::GenUnifId("u_lDir"),
		U_LColor =		GId::GenUnifId("u_lColor");

// ----------- RayleighMie -----------
RayleighMie::RayleighMie() {
	setRayleighCoeff({});
	setMieCoeff(0.8f, 0.1f);
	setLightDir({0,1,0});
	setLightColor({1,1,1});
	setLightPower(1.f);
	setDivide(1e1f);
}
void RayleighMie::outputParams(rs::IEffect& e) const {
	e.setUniform(U_Rayleigh, _rayleigh);
	e.setUniform(U_Mie, spn::Vec2{_mieGain, _mie});
	e.setUniform(U_LDir, _lDir);
	e.setUniform(U_LColor, _lColor);
	e.setUniform(U_LPower, _lPower);
	e.setUniform(U_SdDivide, _sdDivide);
}
void RayleighMie::setRayleighCoeff(const spn::Vec3& c) {
	_rayleigh = c;
}
void RayleighMie::setMieCoeff(const float gain, const float c) {
	_mieGain = gain;
	_mie = c;
}
void RayleighMie::setLightDir(const spn::Vec3& d) {
	_lDir = d;
}
void RayleighMie::setLightColor(const spn::Vec3& c) {
	_lColor = c;
}
void RayleighMie::setLightPower(const float p) {
	_lPower = p;
}
void RayleighMie::setDivide(const float d) {
	_sdDivide = d;
}
