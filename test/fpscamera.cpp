#include "test.hpp"
#include "fpscamera.hpp"
#include "../input.hpp"
#include "../gameloophelper.hpp"
#include "../camera.hpp"

struct FPSCamera::St_Default : StateT<St_Default> {
	void onUpdate(FPSCamera& self) override;
};
void FPSCamera::St_Default::onUpdate(FPSCamera& self) {
	auto lkb = sharedbase.lock();
	auto lk = sharedv.lock();
	// カメラ操作
	auto btn = mgr_input.isKeyPressing(lk->actPress);
	if(btn ^ self._bPress) {
		lkb->hlIm.ref()->setMouseMode((!self._bPress) ? rs::MouseMode::Relative : rs::MouseMode::Absolute);
		self._bPress = btn;
	}
	constexpr float speed = 0.25f;
	float mvF=0, mvS=0;
	if(mgr_input.isKeyPressing(lk->actUp))
		mvF += speed;
	if(mgr_input.isKeyPressing(lk->actDown))
		mvF -= speed;
	if(mgr_input.isKeyPressing(lk->actLeft))
		mvS -= speed;
	if(mgr_input.isKeyPressing(lk->actRight))
		mvS += speed;

	auto& cd = lk->hlCam.ref();
	auto& ps = cd.refPose();
	ps.moveFwd3D(mvF);
	ps.moveSide3D(mvS);
	if(self._bPress) {
		float xv = mgr_input.getKeyValue(lk->actMoveX)/6.f,
			yv = mgr_input.getKeyValue(lk->actMoveY)/6.f;
		self._yaw += spn::DegF(xv);
		self._pitch += spn::DegF(-yv);
		self._yaw.single();
		self._pitch.rangeValue(-89, 89);
		ps.setRot(spn::AQuat::RotationYPR(self._yaw, self._pitch, self._roll));
	}
}
void FPSCamera::initState() {
	setStateNew<St_Default>();
}
FPSCamera::FPSCamera() {
	_bPress = false;
	_yaw = _pitch = _roll = spn::DegF(0);
}
