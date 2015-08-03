#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"
#include "../adaptsdl.hpp"
#include "../gameloophelper.hpp"
#include "../camera.hpp"
#include "../input.hpp"
#include "scene.hpp"
#include "engine.hpp"

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
			lk->pEngine->ref<rs::SystemUniform3D>().setCamera(lk->hlCam);
		}
		// init Input
		{
			// quit[Esc]							アプリケーション終了
			lk->actQuit = mgr_input.makeAction("quit");
			lk->actQuit->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_ESCAPE));
			// reset-scene[R]						シーンリセット
			lk->actReset = mgr_input.makeAction("reset");
			lk->actReset->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_LSHIFT));
			// mode: cube[Z]
			lk->actCube = mgr_input.makeAction("mode_cube");
			lk->actCube->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_Z));
			// mode: sound[X]
			lk->actSound = mgr_input.makeAction("mode_sound");
			lk->actSound->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_X));
			// mode: sprite[C]
			lk->actSprite = mgr_input.makeAction("mode_sprite");
			lk->actSprite->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_C));
			// mode: spriteD[V]
			lk->actSpriteD = mgr_input.makeAction("mode_spriteD");
			lk->actSpriteD->addLink(rs::InF::AsButton(lkb->hlIk, SDL_SCANCODE_V));
			const int c_scancode[] = { SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6};
			for(int i=0 ; i<static_cast<int>(countof(lk->actNumber)) ; i++) {
				lk->actNumber[i] = mgr_input.makeAction((boost::format("number_%1%")%i).str());
				lk->actNumber[i]->addLink(rs::InF::AsButton(lkb->hlIk, c_scancode[i]));
			}
			// left, right, up, down [A,D,W,S]		カメラ移動
			lk->actAx = mgr_input.makeAction("axisX");
			lk->actAy = mgr_input.makeAction("axisY");
			mgr_input.linkButtonAsAxisMulti(
				lkb->hlIk,
				std::make_tuple(lk->actAx, SDL_SCANCODE_A, SDL_SCANCODE_D),
				std::make_tuple(lk->actAy, SDL_SCANCODE_S, SDL_SCANCODE_W)
			);
			// rotate-camera[MouseX,Y]				カメラ向き変更
			lk->actMoveX = mgr_input.makeAction("moveX");
			lk->actMoveY = mgr_input.makeAction("moveY");
			// rotate-switch[MouseLeft]				カメラ回転切り替え
			lk->actPress = mgr_input.makeAction("press");

			lk->actMoveX->addLink(rs::InF::AsAxis(lkb->hlIm, 0));
			lk->actMoveY->addLink(rs::InF::AsAxis(lkb->hlIm, 1));
			lk->actPress->addLink(rs::InF::AsButton(lkb->hlIm, 0));
		}
	};
	return rs::GameloopHelper<Engine, SharedValue, Sc_Base>::Run(cbInit, RESOLUTION_X, RESOLUTION_Y, APP_NAME, argv[1]);
}
