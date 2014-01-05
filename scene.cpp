#include "scene.hpp"

namespace rs {
	bool SceneMgr::isEmpty() const { return _scene.empty(); }
	HGbj SceneMgr::getTop() const {
		if(_scene.empty())
			return HGbj();
		return _scene.top();
	}
	void SceneMgr::setPushScene(HGbj hSc, bool bPop) {
		if(_scene.empty())
			_scene.push(HLGbj(hSc));
		else {
			_scNext = HLGbj(hSc);
			_scNPop = bPop ? 1 : 0;
			_scArg = Variant();
			_scOp = true;
		}
	}
	void SceneMgr::setPopScene(int nPop, const Variant& arg) {
		_scNext = HLGbj();
		_scNPop = nPop;
		_scArg = arg;
		_scOp = true;
	}
	void SceneMgr::_doSceneOp() {
		if(!_scOp && _scene.top().ref()->isDead()) {
			// Sceneがdestroyされていて、かつSceneOpが無ければpop(1)とする
			setPopScene(1);
		}
		while(_scOp) {
			// Update中に指示されたScene操作タスクを実行
			ObjTypeID id = _scene.top().ref()->getTypeID();

			int nPop = std::min(_scNPop, static_cast<int>(_scene.size()));
			_scNPop = 0;
			while(--nPop >= 0) {
				auto& sc = _scene.top().ref();
				sc->destroy();
				sc->onUpdate();			// nullステートへ移行させる
				sc->onDestroy();
				_scene.pop();
			}
			// Sceneスタックが空ならここで終わり
			if(_scene.empty())
				return;

			_scOp = false;
			// 直後に積むシーンがあればそれを積む
			if(_scNext.valid())
				_scene.push(_scNext);
			else {
				// 降りた先のシーンに戻り値を渡す
				_scene.top().ref()->onDown(id, _scArg);
				_scArg = Variant();
			}
		}
	}
	bool SceneMgr::onUpdate() {
		if(_scene.empty())
			return true;

		_scene.top().ref()->onUpdate();
		// SceneOpがあれば処理
		_doSceneOp();
		// スタックが空だったらtrue = 終了の合図 を返す
		return _scene.empty();
	}
	void SceneMgr::onDraw() {
		_scene.top().ref()->onDraw();
		// DrawフェーズでのSceneOpは許可されない
		AssertP(Trap, !_scOp)
	}
	#define DEF_ADAPTOR(name) void SceneMgr::name() { \
		_scene.top().ref()->name(); }
	DEF_ADAPTOR(onPause)
	DEF_ADAPTOR(onStop)
	DEF_ADAPTOR(onResume)
	DEF_ADAPTOR(onReStart)
	#undef DEF_ADAPTOR
}
