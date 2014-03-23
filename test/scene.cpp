#include "test.hpp"

spn::Optional<Cube> g_cube;
const rs::GMessageID MSG_CallFunc = rs::GMessage::RegMsgID("call_base");
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
	self._drawCube();
	CheckQuit();
}
void TScene::MySt::CheckQuit() {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		LogOutput("TScene::onUpdate::Quit");
		mgr_scene.setPopScene(1);
	}
}
void TScene::MySt::onDown(TScene& self, rs::ObjTypeID prevID, const rs::LCValue& arg) {
	LogOutput("TScene::onDown");
}
void TScene::MySt::onPause(TScene& self) {
	LogOutput("TScene::onPause");
}
void TScene::MySt::onResume(TScene& self) {
	LogOutput("TScene::onResume");
}
void TScene::MySt::onStop(TScene& self) {
	LogOutput("TScene::onStop");
}
void TScene::MySt::onReStart(TScene& self) {
	LogOutput("TScene::onReStart");
}
rs::LCValue TScene::MySt::recvMsg(TScene& self, rs::GMessageID msg, const rs::LCValue& arg) {
	if(msg == MSG_CallFunc)
		self._drawCube();
	else if(msg == MSG_GetStatus)
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
	self._drawCube();
}
rs::LCValue TScene::MySt_Play::recvMsg(TScene& self, rs::GMessageID msg, const rs::LCValue& arg) {
	if(msg == MSG_GetStatus)
		return "Playing";
	return rs::LCValue();
}

// ------------------------ TScene ------------------------
TScene::TScene(): Scene(0) {
	LogOutput("TScene::ctor");
	// サウンド読み込み
	spn::PathBlock pb(mgr_path.getPath(rs::AppPath::Type::Sound));
	pb <<= "the_thunder.ogg";
	_hlAb = mgr_sound.loadOggStream(mgr_rw.fromFile(pb.plain_utf8(), rs::RWops::Read, false));
	_hlSg = mgr_sound.createSourceGroup(1);
	// Cube初期化
	spn::URI uriTex("file", mgr_path.getPath(rs::AppPath::Type::Texture));
	uriTex <<= "brick.jpg";
	rs::HLTex hlTex = mgr_gl.loadTexture(uriTex);
	g_cube = spn::construct(1.f, hlTex);

	setStateNew<MySt>();
}
void TScene::_drawCube() {
	auto lk = shared.lock();
	rs::GLEffect& glx = *lk->hlFx.ref();
	g_cube->draw(glx);
}
void TScene::onDestroy() {
	LogOutput("TScene::onDestroy");
	g_cube = spn::none;
}
TScene::~TScene() {
	LogOutput("TScene::dtor");
}

// ------------------------ TScene2 ------------------------
// TScene2::TScene2() {
// 	setStateNew<MySt>();
// 	LogOutput("TScene2::ctor");
// }
// TScene2::~TScene2() {
// 	LogOutput("TScene2::dtor");
// }
// void TScene2::MySt::onUpdate(TScene2& self) {
// 	auto lk = shared.lock();
// 	if(mgr_input.isKeyPressed(lk->actRight)) {
// 		mgr_scene.setPopScene(1);
// 	}
// }
