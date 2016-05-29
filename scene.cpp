#include "scene.hpp"

namespace rs {
	ImplDrawGroup(SceneBase::Draw, 0x0000)
	// -------------- SceneBase --------------
	SceneBase::SceneBase(HGroup hUpd, HDGroup hDraw) {
		_update = hUpd ? hUpd : rs_mgr_obj.makeGroup<Update>().first.get();
		_draw = hDraw ? hDraw : rs_mgr_obj.makeDrawGroup<Draw>().first.get();
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
	IScene& SceneMgr::getSceneInterface(int n) const {
		HScene hSc = getScene(n);
		return *hSc->get();
	}
	HScene SceneMgr::getTop() const {
		return getScene(0);
	}
	HScene SceneMgr::getScene(int n) const {
		if(static_cast<int>(_scene.size()) > n)
			return _scene.at(_scene.size()-1-n);
		return HScene();
	}
	UpdGroup& SceneMgr::getUpdGroupRef(int n) const {
		return *getSceneInterface(n).getUpdGroup()->get();
	}
	DrawGroup& SceneMgr::getDrawGroupRef(int n) const {
		return *getSceneInterface(n).getDrawGroup()->get();
	}
	void SceneMgr::setPushScene(HScene hSc, bool bPop) {
		if(_scene.empty()) {
			_scene.push_back(HLScene(hSc));
			_scene.back()->get()->onConnected(HGroup());
		} else {
			_scNext = HLScene(hSc);
			_scNPop = bPop ? 1 : 0;
			_scArg = LCValue();
			_scOp = true;
		}
	}
	void SceneMgr::setPopScene(int nPop, const LCValue& arg) {
		_scNext = HLScene();
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
				sc->onUpdate(true);			// nullステートへ移行させる
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
		UpdGroup::ProcAddRemove();
		if(_scene.empty())
			return true;

		_scene.back().ref()->onUpdate(true);
		// SceneOpがあれば処理
		_doSceneOp();
		// スタックが空だったらtrue = 終了の合図 を返す
		return _scene.empty();
	}
	void SceneMgr::onDraw(IEffect& e) {
		if(_scene.empty())
			return;

		_scene.back().ref()->onDraw(e);
		// DrawフェーズでのSceneOpは許可されない
		AssertP(Trap, !_scOp)
	}
	#define DEF_ADAPTOR(ret, name) ret SceneMgr::name() { \
		return _scene.front().ref()->name(); }
	DEF_ADAPTOR(bool, onPause)
	DEF_ADAPTOR(void, onStop)
	DEF_ADAPTOR(void, onResume)
	DEF_ADAPTOR(void, onReStart)
	DEF_ADAPTOR(void, onEffectReset)
	#undef DEF_ADAPTOR

	// -------------- U_Scene --------------
	struct U_Scene::St_None : StateT<St_None> {};
	U_Scene::U_Scene() {
		setStateNew<St_None>();
	}
}
