#include "test.hpp"
#include "dwrapper.hpp"
#include "cube.hpp"
#include "scene.hpp"
#include "../input.hpp"

struct Sc_Cube::St_Default : StateT<St_Default> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override;
	void onUpdate(Sc_Base& self) override;
	rs::LCValue recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
rs::LCValue Sc_Cube::St_Default::recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) {
	if(id == MSG_StateName)
		return "Sc_Cube";
	return rs::LCValue();
}
void Sc_Cube::St_Default::onConnected(Sc_Base& self, rs::HGroup hGroup) {
	// 前シーンの描画 & アップデートグループを流用
	{	 auto hDGroup = mgr_scene.getSceneBase(1).getDraw();
		mgr_scene.getSceneBase(0).getDraw()->get()->addObj(hDGroup); }
	{	auto hG = mgr_scene.getSceneBase(1).getUpdate();
		mgr_scene.getSceneBase(0).getUpdate()->get()->addObj(hG); }
	// ---- make cube ----
	rs::HLTex hlTex = mgr_gl.loadTexture("brick.jpg");
	{
		using CubeObj = DWrapper<Cube>;
		rs::HLDObj hlObj = rs_mgr_obj.makeDrawable<CubeObj>(Cube::T_Cube, 1.f, hlTex);
		auto* sp = static_cast<CubeObj*>(hlObj->get());
		sp->setOffset(spn::Vec3(0,0,2));
		self.getBase().getUpdate()->get()->addObj(hlObj.get());
		static_cast<Sc_Cube&>(self)._hCube = hlObj;
	}
}
void Sc_Cube::St_Default::onUpdate(Sc_Base& self) {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actSound))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Sound>(), true);
	else if(mgr_input.isKeyPressed(lk->actSprite))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSort>(), true);
	else
		self.checkQuit();
}
void Sc_Cube::initState() {
	setStateNew<St_Default>();
}
