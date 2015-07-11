#include "test.hpp"
#include "dwrapper.hpp"
#include "cube.hpp"
#include "scene.hpp"

struct Sc_Cube::St_Default : StateT<St_Default> {
	void onConnected(Sc_Cube& self, rs::HGroup hGroup) override;
	void onUpdate(Sc_Cube& self) override;
	rs::LCValue recvMsg(Sc_Cube& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
rs::LCValue Sc_Cube::St_Default::recvMsg(Sc_Cube& self, rs::GMessageId id, const rs::LCValue& arg) {
	if(id == MSG_StateName)
		return "Sc_Cube";
	return rs::LCValue();
}
void Sc_Cube::St_Default::onConnected(Sc_Cube& self, rs::HGroup hGroup) {
	// 前シーンの描画 & アップデートグループを流用
	{	auto hDGroup = mgr_scene.getSceneBase(1).getDraw();
		mgr_scene.getDrawGroup().addObj(hDGroup); }
	{	auto hG = mgr_scene.getSceneBase(1).getUpdate();
		mgr_scene.getUpdGroup().addObj(hG); }
	// ---- make cube ----
	rs::HLTex hlTex = mgr_gl.loadTexture("brick.jpg");
	{
		using CubeObj = DWrapper<Cube>;
		auto hlp = rs_mgr_obj.makeDrawable<CubeObj>(Cube::T_Cube, rs::HDGroup(), 1.f, hlTex);
		hlp.second->setOffset(spn::Vec3(0,0,2));
		self.getBase().getUpdate()->get()->addObj(hlp.first.get());
		self._hCube = hlp.first;
	}
}
void Sc_Cube::St_Default::onUpdate(Sc_Cube& self) {
	self._base.checkSwitchScene();
}
Sc_Cube::Sc_Cube(Sc_Base& b): _base(b) {
	setStateNew<St_Default>();
}
