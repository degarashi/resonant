#include "prochelper.hpp"
#include "glx.hpp"
#include "input.hpp"
#include "sound.hpp"

namespace rs {
	// ------------------------------ DrawProc ------------------------------
	bool DrawProc::runU(uint64_t accum, bool bSkip) {
		auto lk = sharedbase.lock();
		auto& fx = *lk->hlFx.ref();
		if(!bSkip) {
			lk->fps.update();
			mgr_scene.onDraw();
		}
		fx.execTask(bSkip);
		return !bSkip;
	}
	// ------------------------------ MainProc ------------------------------
	MainProc::MainProc(const rs::SPWindow& sp, bool bInitInput):
		_bFirstScene(false)
	{
		auto lk = sharedbase.lock();
		lk->spWindow = sp;
		lk->screenSize *= 0;
		if(bInitInput)
			_initInput();
	}
	void MainProc::_pushFirstScene(rs::HGbj hGbj) {
		Assert(Trap, !_bFirstScene, "pushed first scene twice")
		mgr_scene.setPushScene(hGbj);
		_bFirstScene = true;
	}
	void MainProc::_initInput() {
		// キーバインディング
		auto lk = sharedbase.lock();
		lk->hlIk = rs::Keyboard::OpenKeyboard();
		lk->hlIm = rs::Mouse::OpenMouse(0);
		auto& mouse = *lk->hlIm.ref();
		mouse.setMouseMode(rs::MouseMode::Absolute);
		mouse.setDeadZone(0, 1.f, 0.f);
		mouse.setDeadZone(1, 1.f, 0.f);
	}
	bool MainProc::_beginProc() {
		Assert(Trap, _bFirstScene, "not pushed first scene yet")

		mgr_sound.update();
		// 描画コマンド
		auto lk = sharedbase.lock();
		rs::GLEffect& fx = *lk->hlFx.ref();
		// 描画スレッドの処理が遅過ぎたらここでウェイトがかかる
		fx.beginTask();
		if(mgr_scene.onUpdate())
			return false;

		// 画面のサイズとアスペクト比合わせ
		auto& scs = lk->screenSize;
		auto sz = lk->spWindow->getSize();
		if(scs != sz) {
			scs = sz;
			GL.glViewport(0,0, sz.width, sz.height);
		}
		return true;
	}
	void MainProc::_endProc() {
		auto& fx = sharedbase.lock()->hlFx.ref();
		fx->endTask();
	}
	void MainProc::onPause() {
		mgr_scene.onPause(); }
	void MainProc::onResume() {
		mgr_scene.onResume(); }
	void MainProc::onStop() {
		auto lk = sharedbase.lock();
		lk->hlFx.ref()->clearTask();
		mgr_scene.onStop();
	}
	void MainProc::onReStart() {
		mgr_scene.onReStart(); }
}

