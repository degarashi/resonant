#include "test.hpp"
#include "../updater.hpp"

const rs::GMessageId MSG_GetStatus = rs::GMessage::RegMsgId("get_status");
const rs::IdValue TScene::T_Info = GlxId::GenTechId("TheTech", "P1"),
				TScene::T_Cube = GlxId::GenTechId("TheCube", "P0");
struct TScene::St_Init : StateT<St_Init> {
	void onConnected(TScene& self, rs::HGroup hGroup) override;
	void onDisconnected(TScene& self, rs::HGroup hGroup) override;
};
struct TScene::St_Idle : StateT<St_Idle, St_Init> {
	void onEnter(TScene& self, rs::ObjTypeId prevId) override;
	void onExit(TScene& self, rs::ObjTypeId nextId) override;
	void onUpdate(TScene& self) override;
	void onDown(TScene& self, rs::ObjTypeId prevId, const rs::LCValue& arg) override;
	void onPause(TScene& self) override;
	void onResume(TScene& self) override;
	void onStop(TScene& self) override;
	void onReStart(TScene& self) override;
	rs::LCValue recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) override;
	static void CheckQuit();
};
struct TScene::St_Play : StateT<St_Play, St_Init> {
	void onEnter(TScene& self, rs::ObjTypeId prevId) override;
	void onUpdate(TScene& self) override;
	rs::LCValue recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) override;
};
// ------------------------ TScene::St_Init ------------------------
void TScene::St_Init::onConnected(TScene& self, rs::HGroup hGroup) {
	self.setStateNew<St_Idle>();
}
void TScene::St_Init::onDisconnected(TScene& self, rs::HGroup hGroup) {
	if(self._hInfo) {
		auto* p = self.getBase().getUpdate()->get();
		p->remObj(self._hInfo);
		self._hInfo = rs::HDObj();
	}
	self.setStateNew<St_Init>();
}
// ------------------------ TScene::St_Idle ------------------------
void TScene::St_Idle::onEnter(TScene& self, rs::ObjTypeId prevId) {
	auto& s = self._hlSg.ref();
	s.clear();

	auto lk = shared.lock();
	// ---- make info ----
	rs::HLDObj hlInfo = rs_mgr_obj.makeDrawable<InfoShow>(T_Info);
	self.getBase().getUpdate()->get()->addObj(hlInfo.get());
	self._hInfo = hlInfo;

	// ---- make cube ----
	rs::HLTex hlTex = mgr_gl.loadTexture("brick.jpg");

	rs::HLDObj hlObj = rs_mgr_obj.makeDrawable<CubeObj>(hlTex, T_Cube);
	self.getBase().getUpdate()->get()->addObj(hlObj.get());
	self._hCube = hlObj;
}
void TScene::St_Idle::onExit(TScene& self, rs::ObjTypeId nextId) {
	auto* p = self.getBase().getUpdate()->get();
	p->remObj(self._hCube);
	self._hCube = rs::HDObj();
}
void TScene::St_Idle::onUpdate(TScene& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actPlay)) {
		self.setStateNew<St_Play>();
	}
	CheckQuit();
}
void TScene::St_Idle::CheckQuit() {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		PrintLog;
		mgr_scene.setPopScene(1);
	}
}
void TScene::St_Idle::onDown(TScene& self, rs::ObjTypeId prevId, const rs::LCValue& arg) {
	PrintLog;
}
void TScene::St_Idle::onPause(TScene& self) {
	PrintLog;
}
void TScene::St_Idle::onResume(TScene& self) {
	PrintLog;
}
void TScene::St_Idle::onStop(TScene& self) {
	PrintLog;
}
void TScene::St_Idle::onReStart(TScene& self) {
	PrintLog;
}
rs::LCValue TScene::St_Idle::recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Idle";
	return rs::LCValue();
}

// ------------------------ TScene::St_Play ------------------------
void TScene::St_Play::onEnter(TScene& self, rs::ObjTypeId prevId) {
	auto& s = self._hlSg.ref();
	s.clear();
	s.play(self._hlAb, 0);
}
void TScene::St_Play::onUpdate(TScene& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actStop))
		self.setStateNew<St_Idle>();
	St_Idle::CheckQuit();
}
rs::LCValue TScene::St_Play::recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Playing";
	return rs::LCValue();
}

DefineGroupT(MyDrawGroup, rs::DrawGroup)
// ------------------------ TScene ------------------------
TScene::TScene() {
	// 描画グループを初期化 (Z-sort)
	getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(rs::DSortV{rs::cs_dsort_z_desc}, false));
	// サウンド読み込み
	_hlAb = mgr_sound.loadOggStream("the_thunder.ogg");
	_hlSg = mgr_sound.createSourceGroup(1);
}
TScene::~TScene() {
	PrintLog;
}
void TScene::initState() {
	setStateNew<St_Init>();
}

// ------------------------ TScene2 ------------------------
TScene2::TScene2() {
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
void TScene2::initState() {
	setStateNew<MySt>();
}
