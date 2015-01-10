#include "test.hpp"
#include "../updater.hpp"

const rs::GMessageId MSG_GetStatus = rs::GMessage::RegMsgId("get_status");
// ------------------------ TScene::MySt ------------------------
void TScene::MySt::onEnter(TScene& self, rs::ObjTypeId prevId) {
	auto& s = self._hlSg.ref();
	s.clear();
}
void TScene::MySt::onUpdate(TScene& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actPlay)) {
		self.setStateNew<MySt_Play>();
	}
	CheckQuit();
}
void TScene::MySt::CheckQuit() {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		PrintLog;
		mgr_scene.setPopScene(1);
	}
}
void TScene::MySt::onDown(TScene& self, rs::ObjTypeId prevId, const rs::LCValue& arg) {
	PrintLog;
}
void TScene::MySt::onPause(TScene& self) {
	PrintLog;
}
void TScene::MySt::onResume(TScene& self) {
	PrintLog;
}
void TScene::MySt::onStop(TScene& self) {
	PrintLog;
}
void TScene::MySt::onReStart(TScene& self) {
	PrintLog;
}
rs::LCValue TScene::MySt::recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Idle";
	return rs::LCValue();
}

// ------------------------ TScene::MySt_Play ------------------------
void TScene::MySt_Play::onEnter(TScene& self, rs::ObjTypeId prevId) {
	auto& s = self._hlSg.ref();
	s.clear();
	s.play(self._hlAb, 0);
}
void TScene::MySt_Play::onUpdate(TScene& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actStop))
		self.setStateNew<MySt>();
	MySt::CheckQuit();
}
rs::LCValue TScene::MySt_Play::recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Playing";
	return rs::LCValue();
}

// ------------------------ TScene ------------------------
TScene::TScene(): Scene(0) {
	PrintLog;
	// サウンド読み込み
	spn::PathBlock pb(mgr_path.getPath(rs::AppPath::Type::Sound));
	pb <<= "the_thunder.ogg";
	_hlAb = mgr_sound.loadOggStream(mgr_rw.fromFile(pb.plain_utf8(), rs::RWops::Read));
	_hlSg = mgr_sound.createSourceGroup(1);

	setStateNew<MySt>();
}
void TScene::onConnected(rs::HGroup) {
	auto lk = shared.lock();
	auto& fx = *lk->pFx;
	auto techId = *fx.getTechID("TheCube");
	fx.setTechnique(techId, true);
	auto passId = *fx.getPassID("P0");

	spn::URI uriTex("file", mgr_path.getPath(rs::AppPath::Type::Texture));
	uriTex <<= "brick.jpg";
	rs::HLTex hlTex = mgr_gl.loadTexture(uriTex);

	rs::HLObj hlObj = rs_mgr_obj.makeObj<CubeObj>(hlTex, techId, passId);
	getBase().update->get()->addObj(hlObj);

	techId = *fx.getTechID("TheTech");
	fx.setTechnique(techId, true);
	passId = *fx.getPassID("P1");
	rs::HLObj hlInfo = rs_mgr_obj.makeObj<InfoShow>(techId, passId);
	getBase().update->get()->addObj(hlInfo);
}
void TScene::onDisconnected(rs::HGroup) {
	PrintLog;
}
TScene::~TScene() {
	PrintLog;
}

// ------------------------ TScene2 ------------------------
TScene2::TScene2() {
	setStateNew<MySt>();
	PrintLog;
}
TScene2::~TScene2() {
	PrintLog;
}
void TScene2::MySt::onUpdate(TScene2& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actRight)) {
		mgr_scene.setPopScene(1);
	}
}
