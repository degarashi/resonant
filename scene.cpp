#include "scene.hpp"

namespace rs {
	bool SceneMgr::isEmpty() const { return _scene.empty(); }
	SceneBase& SceneMgr::getSceneBase(int n) const {
		HGbj hGbj = getScene(n);
		auto* ptr = hGbj.ref()->getInterface(SCENE_INTERFACE_ID);
		Assert(Trap, ptr)
		return *static_cast<SceneBase*>(ptr);
	}
	HGbj SceneMgr::getTop() const {
		return getScene(0);
	}
	HGbj SceneMgr::getScene(int n) const {
		if(_scene.size() > n)
			return _scene.at(_scene.size()-1-n);
		return HGbj();
	}
	void SceneMgr::setPushScene(HGbj hSc, bool bPop) {
		if(_scene.empty()) {
			_scene.push_back(HLGbj(hSc));
			_scene.back()->get()->onCreate(nullptr);
		} else {
			_scNext = HLGbj(hSc);
			_scNPop = bPop ? 1 : 0;
			_scArg = LCValue();
			_scOp = true;
		}
	}
	void SceneMgr::setPopScene(int nPop, const LCValue& arg) {
		_scNext = HLGbj();
		_scNPop = nPop;
		_scArg = arg;
		_scOp = true;
	}
	void SceneMgr::_doSceneOp() {
		if(!_scOp && _scene.back().ref()->isDead()) {
			// Sceneがdestroyされていて、かつSceneOpが無ければpop(1)とする
			setPopScene(1);
		}
		while(_scOp) {
			// Update中に指示されたScene操作タスクを実行
			ObjTypeID id = _scene.back().ref()->getTypeID();

			int nPop = std::min(_scNPop, static_cast<int>(_scene.size()));
			_scNPop = 0;
			while(--nPop >= 0) {
				auto& sc = _scene.back().ref();
				sc->destroy();
				sc->onUpdate();			// nullステートへ移行させる
				sc->onDestroy(nullptr);
				_scene.pop_back();
			}
			// Sceneスタックが空ならここで終わり
			if(_scene.empty())
				return;

			_scOp = false;
			// 直後に積むシーンがあればそれを積む
			if(_scNext.valid()) {
				_scene.push_back(_scNext);
				_scNext.ref()->onCreate(nullptr);
				_scNext.release();
			} else {
				// 降りた先のシーンに戻り値を渡す
				_scene.back().ref()->onDown(id, _scArg);
				_scArg = LCValue();
			}
		}
	}
	bool SceneMgr::onUpdate() {
		if(_scene.empty())
			return true;

		_scene.back().ref()->onUpdate();
		// SceneOpがあれば処理
		_doSceneOp();
		// スタックが空だったらtrue = 終了の合図 を返す
		return _scene.empty();
	}
	void SceneMgr::onDraw() {
		if(_scene.empty())
			return;

		_scene.back().ref()->onDraw();
		// DrawフェーズでのSceneOpは許可されない
		AssertP(Trap, !_scOp)
	}
	#define DEF_ADAPTOR(name) void SceneMgr::name() { \
		_scene.front().ref()->name(); }
	DEF_ADAPTOR(onPause)
	DEF_ADAPTOR(onStop)
	DEF_ADAPTOR(onResume)
	DEF_ADAPTOR(onReStart)
	#undef DEF_ADAPTOR
}
