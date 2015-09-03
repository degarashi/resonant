#include "test.hpp"
#include "../util/dwrapper.hpp"
#include "cube.hpp"
#include "scene.hpp"

struct Sc_Cube::St_Default : StateT<St_Default> {
	void onConnected(Sc_Cube& self, rs::HGroup hGroup) override;
	void onUpdate(Sc_Cube& self, const rs::SPLua& ls) override;
	rs::LCValue recvMsg(Sc_Cube& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
rs::LCValue Sc_Cube::St_Default::recvMsg(Sc_Cube& /*self*/, rs::GMessageId id, const rs::LCValue& /*arg*/) {
	if(id == MSG_StateName)
		return "Sc_Cube";
	return rs::LCValue();
}
void Sc_Cube::St_Default::onConnected(Sc_Cube& self, rs::HGroup /*hGroup*/) {
	self.getDrawGroupRef().setSortAlgorithm({rs::cs_dsort_priority_asc}, false);
	// ---- make FBClear ----
	self.getDrawGroupRef().addObj(MakeFBClear(0x0000));
	// ---- make cube ----
	rs::HLTex hlTex = mgr_gl.loadTexture("block.jpg", rs::MipmapLinear);
	{
		using CubeObj = rs::util::DWrapper<Cube>;
		auto hlp = rs_mgr_obj.makeDrawable<CubeObj>(::MakeCallDraw<Engine>(), Cube::T_Cube, rs::HDGroup(), 1.f, hlTex);
		hlp.second->setOffset(spn::Vec3(0,0,2));
		self.getBase().getUpdate()->get()->addObj(hlp.first.get());
		self._hCube = hlp.first;
		hlp.second->setPriority(0x1000);
	}
	// 前シーンの描画 & アップデートグループを流用
	{	auto hDGroup = mgr_scene.getSceneInterface(1).getDrawGroup();
		mgr_scene.getDrawGroupRef().addObj(rs_mgr_obj.makeDrawGroup<MyP>(hDGroup).first.get());
	}
// 		mgr_scene.getDrawGroupRef().addObj(hDGroup); }
	{	auto hG = mgr_scene.getSceneInterface(1).getUpdGroup();
		mgr_scene.getUpdGroupRef().addObj(hG); }
}
void Sc_Cube::St_Default::onUpdate(Sc_Cube& self, const rs::SPLua& /*ls*/) {
	self._base.checkSwitchScene();
}
Sc_Cube::Sc_Cube(Sc_Base& b): _base(b) {
	setStateNew<St_Default>();
}
