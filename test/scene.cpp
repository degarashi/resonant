#include "test.hpp"
#include "../updater.hpp"

const rs::GMessageId MSG_GetStatus = rs::GMessage::RegMsgId("get_status");
const rs::IdValue TScene::T_Info = GlxId::GenTechId("TheTech", "P1"),
				TScene::T_Cube = GlxId::GenTechId("TheCube", "P0");
// ------------------------ TScene::MySt ------------------------
void TScene::MySt::onEnter(TScene& self, rs::ObjTypeId prevId) {
	auto& s = self._hlSg.ref();
	s.clear();

	auto lk = shared.lock();
	// ---- make info ----
	rs::HLDObj hlInfo = rs_mgr_obj.makeDrawable<InfoShow>(T_Info);
	self.getBase().update->get()->addObj(hlInfo.get());
	self._hInfo = hlInfo;

	// ---- make cube ----
	rs::HLTex hlTex = mgr_gl.loadTexture("brick.jpg");

	rs::HLDObj hlObj = rs_mgr_obj.makeDrawable<CubeObj>(hlTex, T_Cube);
	self.getBase().update->get()->addObj(hlObj.get());
	self._hCube = hlObj;
}
void TScene::MySt::onExit(TScene& self, rs::ObjTypeId nextId) {
	auto* p = self.getBase().update->get();
	p->remObj(self._hCube);
	p->remObj(self._hInfo);
	self._hCube = rs::HDObj();
	self._hInfo = rs::HDObj();
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

DefineGroupT(MyDrawGroup, rs::DrawGroup)
// ------------------------ TScene ------------------------
TScene::TScene() {
	// 描画グループを初期化 (Z-sort)
	getBase().draw = rs_mgr_obj.makeDrawGroup<MyDrawGroup>(rs::DSortV{rs::cs_dsort_z_desc}, false);
	// サウンド読み込み
	_hlAb = mgr_sound.loadOggStream("the_thunder.ogg");
	_hlSg = mgr_sound.createSourceGroup(1);

	setStateNew<MySt>();
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
