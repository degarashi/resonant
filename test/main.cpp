#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"
#include "../adaptsdl.hpp"
#include "../gameloophelper.hpp"

namespace {
	constexpr int RESOLUTION_X = 1024,
				RESOLUTION_Y = 768;
	constexpr char APP_NAME[] = "rse_test";
}

int main(int argc, char **argv) {
	// 第一引数にpathlistファイルの指定が必須
	if(argc <= 1) {
		LogOutput("usage: rse_demo pathlist_file");
		return 0;
	}
	auto cbInit = [](){
		auto lkb = sharedbase.lock();
		auto lk = sharedv.lock();
		// init Camera
		{
			lk->hlCam = mgr_cam.emplace();
			auto& cd = lk->hlCam.ref();
			auto& ps = cd.refPose();
			ps.setOffset({0,0,-3});
			cd.setFov(spn::DegF(60));
			cd.setZPlane(0.01f, 500.f);
		}
		// init Effect
		{
			lk->pEngine = static_cast<Engine*>(lkb->hlFx->get());
			lk->pEngine->setCamera(lk->hlCam);
		}
		// init Input
		{
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
		}
	};
	return rs::GameloopHelper<Engine, SharedValue, TScene>::Run(cbInit, RESOLUTION_X, RESOLUTION_Y, APP_NAME, argv[1]);
}
