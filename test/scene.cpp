#include "test.hpp"

// ------------------------ TScene::MySt ------------------------
void TScene::MySt::onEnter(TScene& self, rs::ObjTypeID prevID) {
	spn::PathBlock pb(mgr_path.getPath(rs::AppPath::Type::Sound));
	pb <<= "test.ogg";
	self._hlAb = mgr_sound.loadOggStream(mgr_rw.fromFile(pb.plain_utf8(), rs::RWops::Read, false));
	self._hlSg = mgr_sound.createSourceGroup(1);
}
void TScene::MySt::onUpdate(TScene& self) {
	auto hGbj = rep_gobj.getObj("Player").get();
	if(!hGbj.valid())
		self.destroy();
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actLeft))
		mgr_scene.setPushScene(mgr_gobj.makeObj<TScene2>());
	if(mgr_input.isKeyPressed(lk->actQuit)) {
		LogOutput("TScene::onUpdate::Quit");
		mgr_scene.setPopScene(1);
	}
}
void TScene::MySt::onDown(TScene& self, rs::ObjTypeID prevID, const rs::Variant& arg) {
	LogOutput("TScene::onDown");
	auto& s = self._hlSg.ref();
	s.clear();
	s.play(self._hlAb, 0);
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

// ------------------------ TScene ------------------------
TScene::TScene(): Scene(0) {
	setStateNew<MySt>();
	LogOutput("TScene::ctor");
	spn::URI uriTex("file", mgr_path.getPath(rs::AppPath::Type::Texture));
	uriTex <<= "test.png";
	rs::HLGbj hg = mgr_gobj.makeObj<CubeObj>(mgr_gl.loadTexture(uriTex).get());
	rep_gobj.setObj("Player", hg.weak());
	_update.addObj(0, hg.get());
	
	rs::HLUpd hlUpd = mgr_upd.emplace(new rs::UpdChild);
	rep_upd.setObj("hello", hlUpd);
}
void TScene::onDestroy() {
	LogOutput("TScene::onDestroy");
}
TScene::~TScene() {
	LogOutput("TScene::dtor");
}

// ------------------------ TScene2 ------------------------
TScene2::TScene2() {
	setStateNew<MySt>();
	LogOutput("TScene2::ctor");
}
TScene2::~TScene2() {
	LogOutput("TScene2::dtor");
}
void TScene2::MySt::onUpdate(TScene2& self) {
	auto lk = shared.lock();
	if(mgr_input.isKeyPressed(lk->actRight))
		mgr_scene.setPopScene(1);
}
