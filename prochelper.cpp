#include "prochelper.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "spinner/unituple/operator.hpp"
#include "spinner/structure/profiler.hpp"
#include "glx_if.hpp"
#include "systeminfo.hpp"

namespace rs {
	// ------------------------------ DrawProc ------------------------------
	bool DrawProc::runU(uint64_t /*accum*/, bool bSkip) {
		auto lk = sharedbase.lock();
		IEffect& fx = *lk->hlFx.ref();
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
	void MainProc::_pushFirstScene(rs::HScene hSc) {
		Assert(Trap, !_bFirstScene, "pushed first scene twice")
		mgr_scene.setPushScene(hSc);
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
			auto lk = sharedbase.lockR();
			auto& fx= *lk->hlFx.cref();
			lk.unlock();
			mgr_scene.onDraw(fx);
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
		SPLua ls;
		{
			auto lk = sharedbase.lockR();
			fx = lk->hlFx->get();
			ls = lk->spLua;
		}
		// 画面のサイズとアスペクト比合わせ
		{
			auto lk = sharedbase.lockR();
			auto& scs = lk->screenSize;
			auto sz = lk->spWindow->getSize();
			if(scs != sz) {
				auto lkw = sharedbase.lock();
				lkw->screenSize = sz;
				GL.glViewport(0,0, sz.width, sz.height);
			}
		}
		// 描画スレッドの処理が遅過ぎたらここでウェイトがかかる
		fx->beginTask();
{ auto p = spn::profiler.beginBlockObj("scene_update");
		if(mgr_scene.onUpdate(ls)) {
			_endProc();
			return false;
		}
}
		return true;
	}
	void MainProc::_endProc() {
		auto lk = sharedbase.lock();
		IEffect& fx = *lk->hlFx.cref();
		fx.endTask();
		lk->diffCount = fx.getDifference();

		mgr_info.setInfo(lk->screenSize, lk->fps.getFPS());
	}
	void MainProc::onPause() {
		mgr_scene.onPause(); }
	void MainProc::onResume() {
		mgr_scene.onResume(); }
	void MainProc::onStop() {
		auto lk = sharedbase.lockR();
		IEffect& e = *lk->hlFx.cref();
		e.clearTask();
		mgr_scene.onStop();
	}
	void MainProc::onReStart() {
		mgr_scene.onReStart(); }
}

