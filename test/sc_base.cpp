#include "test.hpp"
#include "scene.hpp"
#include "infoshow.hpp"
#include "profileshow.hpp"
#include "fpscamera.hpp"
#include "../updater.hpp"
#include "../gameloophelper.hpp"
#include "../font.hpp"
#include "../util/screen.hpp"

ImplDrawGroup(BaseDRG, 0x8000)
const rs::GMessageId MSG_StateName = rs::GMessage::RegMsgId("MSG_StateName");
namespace {
	constexpr int RandomId = 0x20000000;
}
struct Sc_Base::St_Default : StateT<St_Default> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override;
	void onDisconnected(Sc_Base& self, rs::HGroup hGroup) override;
};
rs::HLDObj MakeFBClear(rs::Priority priority) {
	rs::draw::ClearParam clp{spn::Vec4{0,0,0,0}, 1.f};
	return rs_mgr_obj.makeDrawable<rs::util::FBClear>(priority, clp).first;
}
void Sc_Base::St_Default::onConnected(Sc_Base& self, rs::HGroup hGroup) {
	mgr_random.initEngine(RandomId);
	self._random = mgr_random.get(RandomId);

	// ---- FBClear(Z) ----
	rs::draw::ClearParam clp{spn::none, 1.f};
	self.getDrawGroup().addObj(rs_mgr_obj.makeDrawable<rs::util::FBClear>(0x0000, clp).first);

	auto hDg = self.getBase().getDraw();
	// ---- make InfoShow ----
	rs::HLDObj hlInfo = rs_mgr_obj.makeDrawable<InfoShow>(hDg).first;
	auto* upd = self.getBase().getUpdate()->get();
	upd->addObj(hlInfo.get());
	self._hInfo = hlInfo;

	rs::CCoreID cid = mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, 5, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Point));
	// ---- make ProfileShow ----
	auto hlProf = rs_mgr_obj.makeDrawable<ProfileShow>(cid, hDg);
	upd->addObj(hlProf.first.get());
	hlProf.second->setOffset({0, 480});

	auto hlFP = rs_mgr_obj.makeObj<FPSCamera>().first;
	self.getBase().getUpdate()->get()->addObj(hlFP);

	mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(self).first, false);
}
void Sc_Base::St_Default::onDisconnected(Sc_Base& self, rs::HGroup hGroup) {
	self._random = spn::none;
	mgr_random.removeEngine(RandomId);

	auto* p = self.getBase().getUpdate()->get();
	p->remObj(self._hInfo);
	self._hInfo = rs::HDObj();
}
Sc_Base::Sc_Base():
	Scene(rs_mgr_obj.makeGroup<BaseUPG>().first,
			rs_mgr_obj.makeDrawGroup<BaseDRG>().first)
{
	setStateNew<St_Default>();
}
spn::MTRandom& Sc_Base::getRand() {
	return *_random;
}
#include "../input.hpp"
void Sc_Base::checkSwitchScene() {
	auto lk = sharedv.lock();
	if(mgr_input.isKeyPressed(lk->actCube))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(*this).first, true);
	else if(mgr_input.isKeyPressed(lk->actSound))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Sound>(*this).first, true);
	else if(mgr_input.isKeyPressed(lk->actSprite))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSort>(*this).first, true);
	else if(mgr_input.isKeyPressed(lk->actSpriteD))
		mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_DSortD>(*this).first, true);
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
