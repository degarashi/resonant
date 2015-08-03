#include "prochelper.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "spinner/unituple/operator.hpp"
#include "spinner/structure/profiler.hpp"
#include "glx_if.hpp"

namespace rs {
	// ------------------------------ DrawProc ------------------------------
	bool DrawProc::runU(uint64_t /*accum*/, bool bSkip) {
		auto lk = sharedbase.lock();
		IEffect& fx = *lk->pEffect;
		if(!bSkip) {
			lk->fps.update();
			lk.unlock();
			fx.execTask();
		}
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
	void MainProc::_pushFirstScene(rs::HObj hObj) {
		Assert(Trap, !_bFirstScene, "pushed first scene twice")
		mgr_scene.setPushScene(hObj);
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

	bool MainProc::runU(Query& q) {
		if(!MainProc::_beginProc())
			return false;

		bool b = userRunU();
{ auto p = spn::profiler.beginBlockObj("scene_draw");
		if(b && q.canDraw()) {
			auto lk = sharedbase.lock();
			auto* ptr = lk->hlFx->get();
			lk.unlock();
			mgr_scene.onDraw(*ptr);
			q.setDraw(true);
		}
}
		_endProc();
		return b;
	}
	bool MainProc::_beginProc() {
		Assert(Trap, _bFirstScene, "not pushed first scene yet")

{ auto p = spn::profiler.beginBlockObj("sound_update");
		mgr_sound.update();
}
		// 描画コマンド
		rs::IEffect* fx;
		{
			auto lk = sharedbase.lock();
			fx = lk->pEffect;
		}
		// 描画スレッドの処理が遅過ぎたらここでウェイトがかかる
		fx->beginTask();
{ auto p = spn::profiler.beginBlockObj("scene_update");
		if(mgr_scene.onUpdate()) {
			_endProc();
			return false;
		}
}

		// 画面のサイズとアスペクト比合わせ
		{
			auto lk = sharedbase.lock();
			auto& scs = lk->screenSize;
			auto sz = lk->spWindow->getSize();
			if(scs != sz) {
				scs = sz;
				GL.glViewport(0,0, sz.width, sz.height);
			}
		}
		return true;
	}
	void MainProc::_endProc() {
		auto lk = sharedbase.lock();
		IEffect& fx = *lk->pEffect;
		fx.endTask();
		lk->diffCount = fx.getDifference();
	}
	void MainProc::onPause() {
		mgr_scene.onPause(); }
	void MainProc::onResume() {
		mgr_scene.onResume(); }
	void MainProc::onStop() {
		auto lk = sharedbase.lock();
		IEffect& e = *lk->pEffect;
		e.clearTask();
		mgr_scene.onStop();
	}
	void MainProc::onReStart() {
		mgr_scene.onReStart(); }
}

