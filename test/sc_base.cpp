#include "test.hpp"
#include "scene.hpp"
#include "infoshow.hpp"
#include "profileshow.hpp"
#include "fpscamera.hpp"
#include "../updater.hpp"
#include "../gameloophelper.hpp"
#include "../font.hpp"

const rs::GMessageId MSG_StateName = rs::GMessage::RegMsgId("MSG_StateName");
namespace {
	constexpr int RandomId = 0x20000000;
}
struct Sc_Base::St_Default : StateT<St_Default> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override;
	void onDisconnected(Sc_Base& self, rs::HGroup hGroup) override;
};
void Sc_Base::St_Default::onConnected(Sc_Base& self, rs::HGroup hGroup) {
	mgr_random.initEngine(RandomId);
	self._random = mgr_random.get(RandomId);

	// ---- make InfoShow ----
	rs::HLDObj hlInfo = rs_mgr_obj.makeDrawable<InfoShow>();
	auto* upd = self.getBase().getUpdate()->get();
	upd->addObj(hlInfo.get());
	self._hInfo = hlInfo;

	rs::CCoreID cid = mgr_text.makeCoreID("IPAGothic", rs::CCoreID(0, 5, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Point));
	// ---- make ProfileShow ----
	auto hlProf = rs_mgr_obj.makeDrawable<ProfileShow>(cid);
	upd->addObj(hlProf.get());

	auto hlFP = rs_mgr_obj.makeObj<FPSCamera>();
	self.getBase().getUpdate()->get()->addObj(hlFP);

	mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(self), false);
}
void Sc_Base::St_Default::onDisconnected(Sc_Base& self, rs::HGroup hGroup) {
	self._random = spn::none;
	mgr_random.removeEngine(RandomId);

	auto* p = self.getBase().getUpdate()->get();
	p->remObj(self._hInfo);
	self._hInfo = rs::HDObj();
}
void Sc_Base::initState() {
	setStateNew<St_Default>();
}
spn::MTRandom& Sc_Base::getRand() {
	return *_random;
}
#include "../input.hpp"
void Sc_Base::checkSwitchScene() {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actCube))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(*this), true);
	else if(mgr_input.isKeyPressed(lk->actSound))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Sound>(*this), true);
	else if(mgr_input.isKeyPressed(lk->actSprite))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSort>(*this), true);
	else if(mgr_input.isKeyPressed(lk->actSpriteD))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSortD>(*this), true);
	else
		checkQuit();
}
#include "../input.hpp"
void Sc_Base::checkQuit() {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		mgr_scene.setPopScene(100);
	}
}
