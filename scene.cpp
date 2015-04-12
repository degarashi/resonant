#include "scene.hpp"

namespace rs {
	// -------------- SceneBase --------------
	SceneBase::SceneBase(HGroup hUpd, HDGroup hDraw) {
		_update = hUpd ? hUpd : rs_mgr_obj.makeGroup<Update>().get();
		_draw = hDraw ? hDraw : rs_mgr_obj.makeDrawGroup<Draw>().get();
	}
	void SceneBase::setUpdate(HGroup hGroup) {
		if(_update)
			_update->get()->onDisconnected(HGroup());
		_update = hGroup;
		if(hGroup)
			hGroup->get()->onConnected(HGroup());
	}
	HGroup SceneBase::getUpdate() const {
		return _update;
	}
	void SceneBase::setDraw(HDGroup hdGroup) {
		// DrawGroupはConnected系の通知を行わない
		_draw = hdGroup;
	}
	HDGroup SceneBase::getDraw() const {
		return _draw;
	}

	// -------------- SceneMgr --------------
	bool SceneMgr::isEmpty() const { return _scene.empty(); }
	SceneBase& SceneMgr::getSceneBase(int n) const {
		HObj hObj = getScene(n);
		auto* ptr = hObj.ref()->getInterface(SCENE_INTERFACE_ID);
		Assert(Trap, ptr)
		return *static_cast<SceneBase*>(ptr);
	}
	HObj SceneMgr::getTop() const {
		return getScene(0);
	}
	HObj SceneMgr::getScene(int n) const {
		if(_scene.size() > n)
			return _scene.at(_scene.size()-1-n);
		return HObj();
	}
	UpdGroup& SceneMgr::getUpdGroup(int n) const {
		return *getSceneBase(n).getUpdate()->get();
	}
	DrawGroup& SceneMgr::getDrawGroup(int n) const {
		return *getSceneBase(n).getDraw()->get();
	}
	void SceneMgr::setPushScene(HObj hSc, bool bPop) {
		if(_scene.empty()) {
			_scene.push_back(HLObj(hSc));
			_scene.back()->get()->onConnected(HGroup());
		} else {
			_scNext = HLObj(hSc);
			_scNPop = bPop ? 1 : 0;
			_scArg = LCValue();
			_scOp = true;
		}
	}
	void SceneMgr::setPopScene(int nPop, const LCValue& arg) {
		_scNext = HLObj();
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
			ObjTypeId id = _scene.back().ref()->getTypeId();

			int nPop = std::min(_scNPop, static_cast<int>(_scene.size()));
			_scNPop = 0;
			while(--nPop >= 0) {
				auto& sc = _scene.back().ref();
				sc->onDisconnected(HGroup());
				sc->destroy();
				sc->onUpdate();			// nullステートへ移行させる
				_scene.pop_back();
			}
			// Sceneスタックが空ならここで終わり
			if(_scene.empty())
				return;

			_scOp = false;
			// 直後に積むシーンがあればそれを積む
			if(_scNext.valid()) {
				_scene.push_back(_scNext);
				_scNext.ref()->onConnected(HGroup());
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
