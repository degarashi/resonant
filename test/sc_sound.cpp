#include "test.hpp"
#include "scene.hpp"
#include "../input.hpp"
#include "../sound.hpp"
#include "../gameloophelper.hpp"

struct Sc_Sound::St_Default : StateT<St_Default> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override;
	void onUpdate(Sc_Base& self) override;
	rs::LCValue recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
rs::LCValue Sc_Sound::St_Default::recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) {
	if(id == MSG_StateName)
		return "Sc_Sound";
	return rs::LCValue();
}
void Sc_Sound::St_Default::onConnected(Sc_Base& self, rs::HGroup hGroup) {
	// 前シーンの描画 & アップデートグループを流用
	{	 auto hDGroup = mgr_scene.getSceneBase(1).getDraw();
		mgr_scene.getSceneBase(0).getDraw()->get()->addObj(hDGroup); }
	{	auto hG = mgr_scene.getSceneBase(1).getUpdate();
		mgr_scene.getSceneBase(0).getUpdate()->get()->addObj(hG); }

	auto& ths = static_cast<Sc_Sound&>(self);
	// サウンド読み込み
	ths._hlAb = mgr_sound.loadOggStream("the_thunder.ogg");
	ths._hlSg = mgr_sound.createSourceGroup(1);

	auto& s = ths._hlSg.ref();
	s.clear();
	s.play(ths._hlAb, 0);
}
void Sc_Sound::St_Default::onUpdate(Sc_Base& self) {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actCube))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(), true);
	else if(mgr_input.isKeyPressed(lk->actSprite))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSort>(), true);
	else
		self.checkQuit();
}
void Sc_Sound::initState() {
	setStateNew<St_Default>();
}
