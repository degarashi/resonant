#include "test.hpp"
#include "../updater.hpp"

const rs::GMessageID MSG_GetStatus = rs::GMessage::RegMsgID("get_status");
// ------------------------ TScene::MySt ------------------------
void TScene::MySt::onEnter(TScene& self, rs::ObjTypeID prevID) {
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
void TScene::MySt::onDown(TScene& self, rs::ObjTypeID prevID, const rs::LCValue& arg) {
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
rs::LCValue TScene::MySt::recvMsg(TScene& self, rs::GMessageID msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Idle";
	return rs::LCValue();
}

// ------------------------ TScene::MySt_Play ------------------------
void TScene::MySt_Play::onEnter(TScene& self, rs::ObjTypeID prevID) {
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
rs::LCValue TScene::MySt_Play::recvMsg(TScene& self, rs::GMessageID msg, const rs::LCValue& arg) {
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

	auto& sb = getBase();
	sb.update_m.setObj("default", sb.update.getChild());
	sb.draw_m.setObj("default", sb.draw.getChild());
	setStateNew<MySt>();
}
void TScene::onCreate(rs::UpdChild*) {
	auto lk = shared.lock();
	auto& fx = *lk->pFx;
	auto techId = *fx.getTechID("TheCube");
	fx.setTechnique(techId, true);
	auto passId = *fx.getPassID("P0");

	spn::URI uriTex("file", mgr_path.getPath(rs::AppPath::Type::Texture));
	uriTex <<= "brick.jpg";
	rs::HLTex hlTex = mgr_gl.loadTexture(uriTex);

	rs::HLGbj hlGbj = mgr_gobj.makeObj<CubeObj>(hlTex, techId, passId);
	getBase().draw.addObj(0x00, hlGbj);

	techId = *fx.getTechID("TheTech");
	fx.setTechnique(techId, true);
	passId = *fx.getPassID("P1");
	rs::HLGbj hlInfo = mgr_gobj.makeObj<InfoShow>(techId, passId);
	getBase().draw.addObj(0x00, hlInfo);
}
void TScene::onDestroy(rs::UpdChild*) {
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
