#include "test.hpp"
#include "scene.hpp"
#include "../sound.hpp"
#include "../gameloophelper.hpp"

struct Sc_Sound::St_Default : StateT<St_Default> {
	void onConnected(Sc_Sound& self, rs::HGroup hGroup) override;
	void onUpdate(Sc_Sound& self) override;
	rs::LCValue recvMsg(Sc_Sound& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
rs::LCValue Sc_Sound::St_Default::recvMsg(Sc_Sound& /*self*/, rs::GMessageId id, const rs::LCValue& /*arg*/) {
	if(id == MSG_StateName)
		return "Sc_Sound";
	return rs::LCValue();
}
void Sc_Sound::St_Default::onConnected(Sc_Sound& self, rs::HGroup /*hGroup*/) {
	// ---- make FBClear ----
	self.getDrawGroupRef().addObj(MakeFBClear(0x0000));
	// 前シーンの描画 & アップデートグループを流用
	{	 auto hDGroup = mgr_scene.getSceneInterface(1).getDrawGroup();
		mgr_scene.getSceneInterface(0).getDrawGroup()->get()->addObj(hDGroup); }
	{	auto hG = mgr_scene.getSceneInterface(1).getUpdGroup();
		mgr_scene.getSceneInterface(0).getUpdGroup()->get()->addObj(hG); }

	// サウンド読み込み
	self._hlAb = mgr_sound.loadOggStream("test_music.ogg");
	self._hlSg = mgr_sound.createSourceGroup(1);

	auto& s = self._hlSg.ref();
	s.clear();
	s.play(self._hlAb, 0);
}
void Sc_Sound::St_Default::onUpdate(Sc_Sound& self) {
	self._base.checkSwitchScene();
}
Sc_Sound::Sc_Sound(Sc_Base& b): _base(b) {
	setStateNew<St_Default>();
}
