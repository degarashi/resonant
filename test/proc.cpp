#include "test.hpp"
#include "font.hpp"
#include "sound.hpp"
#include "glresource.hpp"
#include "camera.hpp"
#include "input.hpp"
#include "adaptsdl.hpp"

// ------------------------------ MyDraw ------------------------------
bool MyDraw::runU(uint64_t accum, bool bSkip) {
	if(!bSkip) {
		GL.glClearColor(0,0,0.1f,1);
		GL.glClearDepth(1.0f);
		GL.glDepthMask(GL_TRUE);
		GL.glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		GL.glDepthMask(GL_FALSE);
	}
	return DrawProc::runU(accum, bSkip);
}

// ------------------------------ MyMain ------------------------------
MyMain::MyMain(const rs::SPWindow& sp): MainProc(sp, true) {
	auto lkb = sharedbase.lock();
	lkb->hlIk.ref();
	rs::SPUriHandler sph = std::make_shared<rs::UriH_File>(u8"/");
	mgr_rw.getHandler().addHandler(0x00, sph);

	_initInput();
	_initCam();
	_initEffect();
	_pushFirstScene(mgr_gobj.makeObj<TScene>());
}
void MyMain::_initEffect() {
	auto lkb = sharedbase.lock();
	auto lk = shared.lock();
	spn::URI uriFx("file", mgr_path.getPath(rs::AppPath::Type::Effect));
	uriFx <<= "test.glx";
	lkb->hlFx = mgr_gl.loadEffect(uriFx, [](rs::AdaptSDL& as){ return new rs::GLEffect(as); });
	auto& pFx = *lkb->hlFx.ref();
	lk->pFx = &pFx;
}
void MyMain::_initCam() {
	auto lk = shared.lock();
	lk->hlCam = mgr_cam.emplace();
	rs::CamData& cd = lk->hlCam.ref();
	cd.setOffset({0,0,-3});
	cd.setFOV(spn::DegF(60));
	cd.setZPlane(0.01f, 500.f);
}
void MyMain::_initInput() {
	auto lkb = sharedbase.lock();
	auto lk = shared.lock();
	// quit[Esc]							アプリケーション終了
	lk->actQuit = mgr_input.addAction("quit");
	mgr_input.link(lk->actQuit, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_ESCAPE));
	// reset-scene[R]						シーンリセット
	lk->actReset = mgr_input.addAction("reset");
	mgr_input.link(lk->actReset, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_LSHIFT));
	// playsound[Q]							音楽再生
	lk->actPlay = mgr_input.addAction("play");
	mgr_input.link(lk->actPlay, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_Q));
	// stopsound[E]							音楽停止
	lk->actStop = mgr_input.addAction("stop");
	lkb->hlIk.ref();
	mgr_input.link(lk->actStop, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_E));
	// left, right, up, down [A,D,W,S]		カメラ移動
	lk->actLeft = mgr_input.addAction("left");
	lk->actRight = mgr_input.addAction("right");
	lk->actUp = mgr_input.addAction("up");
	lk->actDown = mgr_input.addAction("down");
	lkb->hlIk.ref();
	mgr_input.link(lk->actLeft, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_A));
	mgr_input.link(lk->actRight, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_D));
	mgr_input.link(lk->actUp, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_W));
	mgr_input.link(lk->actDown, rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_S));
	// rotate-camera[MouseX,Y]				カメラ向き変更
	lk->actMoveX = mgr_input.addAction("moveX");
	lk->actMoveY = mgr_input.addAction("moveY");
	// rotate-switch[MouseLeft]				カメラ回転切り替え
	lk->actPress = mgr_input.addAction("press");

	mgr_input.link(lk->actMoveX, rs::InF::AsAxis(lkb->hlIm, 0));
	mgr_input.link(lk->actMoveY, rs::InF::AsAxis(lkb->hlIm, 1));
	mgr_input.link(lk->actPress, rs::InF::AsButton(lkb->hlIm, 0));
	_bPress = false;
}
bool MyMain::userRunU() {
	auto lkb = sharedbase.lock();
	auto lk = shared.lock();
	// カメラ操作
	auto btn = mgr_input.isKeyPressing(lk->actPress);
	if(btn ^ _bPress) {
		lkb->hlIm.ref()->setMouseMode((!_bPress) ? rs::MouseMode::Relative : rs::MouseMode::Absolute);
		_bPress = btn;
	}
	constexpr float speed = 0.25f;
	float mvF=0, mvS=0;
	if(mgr_input.isKeyPressing(lk->actUp))
		mvF += speed;
	if(mgr_input.isKeyPressing(lk->actDown))
		mvF -= speed;
	if(mgr_input.isKeyPressing(lk->actLeft))
		mvS -= speed;
	if(mgr_input.isKeyPressing(lk->actRight))
		mvS += speed;

	auto& cd = lk->hlCam.ref();
	cd.moveFwd3D(mvF);
	cd.moveSide3D(mvS);
	if(_bPress) {
		float xv = mgr_input.getKeyValue(lk->actMoveX)/4.f,
			yv = mgr_input.getKeyValue(lk->actMoveY)/4.f;
		cd.addRot(spn::Quat::RotationY(spn::DegF(-xv)));
		cd.addRot(spn::Quat::RotationX(spn::DegF(-yv)));
	}
	return true;
}

