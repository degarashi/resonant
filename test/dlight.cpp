#include "dlight.hpp"
#include "../camera.hpp"
#include "../glx_if.hpp"

uint32_t DLight::GetterC::operator()(const rs::HLCamF& hlc, Camera*, const DLight&) const {
	return hlc->getAccum();
}
DLight::DLight() {
	setPosition(spn::Vec3(0,0,0));
	setDirection(spn::Vec3(0,0,1));
	setColor(spn::Vec3(1,1,1));
	setDepthRange(spn::Vec2(0, 10));
}
spn::RFlagRet DLight::_refresh(spn::Mat44& m, Matrix*) const {
	auto& c = getCamera().cref();
	m = c.getViewProj();
	return {true, 0};
}
spn::RFlagRet DLight::_refresh(typename ScLit::value_type& sclit, ScLit*) const {
	auto ret = _rflag.getWithCheck(this, sclit);
	bool b = ret.second;
	if(b) {
		auto& cam = getEyeCamera().cref();
		auto& mView = cam.getView();
		sclit.pos = std::get<0>(ret.first)->asVec4(1) * mView;
		sclit.dir = std::get<1>(ret.first)->asVec4(0) * mView;
	}
	return {b, 0};
}
spn::RFlagRet DLight::_refresh(typename IVP::value_type& ivp, IVP*) const {
	auto ret = _rflag.getWithCheck(this, ivp);
	bool b = ret.second;
	if(b) {
		// Invert(View) * Light(ViewProj)
		auto& cam = std::get<0>(ret.first)->cref();
		auto mView = cam.getView().convertA44();
		mView.invert();
		ivp = mView * *std::get<1>(ret.first);
	}
	return {b, 0};
}
spn::RFlagRet DLight::_refresh(rs::HLCamF& c, Camera*) const {
	c = mgr_cam.emplace();
	auto& pose = c->refPose();
	pose.setOffset(getPosition());
	spn::AQuat q;
	if(getDirection().z < -1.f+1e-4f)
		q = spn::AQuat::RotationY(spn::DegF(180.f));
	else
		q = spn::AQuat::Rotation({0,0,1}, getDirection());
	pose.setRot(q);
	c->setFov(spn::DegF(90));
	auto r = getDepthRange();
	c->setZPlane(r.x, r.y);
	return {true, 0};
}
void DLight::prepareUniforms(rs::IEffect& e) const {
	#define DEF_SETUNIF(name, func) \
		if(auto idv = e.getUnifId(myunif::name)) \
			e.setUniform(*idv, func(), true);
	DEF_SETUNIF(U_Position, getPosition)
	DEF_SETUNIF(U_Color, getColor)
	DEF_SETUNIF(U_Dir, getDirection)
	DEF_SETUNIF(U_DepthRange, getDepthRange)
	DEF_SETUNIF(U_Mat, getMatrix)

	DEF_SETUNIF(U_Coeff, getCoeff)
	DEF_SETUNIF(U_LightIVP, getIVP)
	#undef DEF_SETUNIF
	{
		auto& scl = getScLit();
		if(auto idv = e.getUnifId(myunif::U_ScrLightPos))
			e.setUniform(*idv, scl.pos, true);
		if(auto idv = e.getUnifId(myunif::U_ScrLightDir))
			e.setUniform(*idv, scl.dir, true);
	}
}
