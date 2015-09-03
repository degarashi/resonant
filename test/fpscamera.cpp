#include "test.hpp"
#include "fpscamera.hpp"
#include "../input.hpp"
#include "../gameloophelper.hpp"
#include "../camera.hpp"

struct FPSCamera::St_Default : StateT<St_Default> {
	void onUpdate(FPSCamera& self, const rs::SPLua& ls) override;
};
void FPSCamera::St_Default::onUpdate(FPSCamera& self, const rs::SPLua& /*ls*/) {
	auto lkb = sharedbase.lockR();
	auto lk = sharedv.lock();
	// カメラ操作
	auto btn = mgr_input.isKeyPressing(lk->actPress);
	if(btn ^ self._bPress) {
		lkb->hlIm.cref()->setMouseMode((!self._bPress) ? rs::MouseMode::Relative : rs::MouseMode::Absolute);
		self._bPress = btn;
	}
	constexpr float speed = 0.25f;
	auto mv = mgr_input.getKeyValueSimplifiedMulti(lk->actAx, lk->actAy);
	float mvF = speed * std::get<1>(mv),
		  mvS = speed * std::get<0>(mv);
	auto& cd = lk->hlCam.ref();
	auto& ps = cd.refPose();
	ps.moveFwd3D(mvF);
	ps.moveSide3D(mvS);
	if(self._bPress) {
		float xv = lk->actMoveX->getValue()/6.f,
			yv = lk->actMoveY->getValue()/6.f;
		self._yaw += spn::DegF(xv);
		self._pitch += spn::DegF(-yv);
		self._yaw.single();
		self._pitch.rangeValue(-89, 89);
		ps.setRot(spn::AQuat::RotationYPR(self._yaw, self._pitch, self._roll));
	}
}
FPSCamera::FPSCamera() {
	_bPress = false;
	_yaw = _pitch = _roll = spn::DegF(0);
	setStateNew<St_Default>();
}
