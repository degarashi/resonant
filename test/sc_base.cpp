#include "test.hpp"
#include "scene.hpp"
#include "infoshow.hpp"
#include "fpscamera.hpp"
#include "../updater.hpp"
#include "../gameloophelper.hpp"

const rs::GMessageId MSG_StateName = rs::GMessage::RegMsgId("MSG_StateName");
struct Sc_Base::St_Default : StateT<St_Default> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override;
	void onDisconnected(Sc_Base& self, rs::HGroup hGroup) override;
	rs::LCValue recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) override;
};
void Sc_Base::St_Default::onConnected(Sc_Base& self, rs::HGroup hGroup) {
	rs::HLDObj hlInfo = rs_mgr_obj.makeDrawable<InfoShow>();
	self.getBase().getUpdate()->get()->addObj(hlInfo.get());
	self._hInfo = hlInfo;

	auto hlFP = rs_mgr_obj.makeObj<FPSCamera>();
	self.getBase().getUpdate()->get()->addObj(hlFP);

	rs::HLObj hlCube = rs_mgr_obj.makeObj<Sc_Cube>();
	mgr_scene.setPushScene(hlCube);
}
void Sc_Base::St_Default::onDisconnected(Sc_Base& self, rs::HGroup hGroup) {
	auto* p = self.getBase().getUpdate()->get();
	p->remObj(self._hInfo);
	self._hInfo = rs::HDObj();
}
rs::LCValue Sc_Base::St_Default::recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) {
	if(id == MSG_StateName)
		return "Sc_Base";
	return rs::LCValue();
}
#include "../input.hpp"
void Sc_Base::checkQuit() {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		mgr_scene.setPopScene(100);
	}
}
void Sc_Base::initState() {
	setStateNew<St_Default>();
}
