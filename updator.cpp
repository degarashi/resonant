#include "updator.hpp"

namespace rs {
	ObjID GenerateObjTypeID() {
		static ObjID s_id(0);
		return s_id++;
	}
	GMessage& GMessage::Ref() {
		static GMessage m;
		return m;
	}
	GMessageID GMessage::RegMsgID(const GMessageStr& msg) {
		GMessage& m = Ref();
		auto itr = m._msgMap.find(msg);
		if(itr != m._msgMap.end())
			return itr->second;
		auto id = m._msgIDCur++;
		m._msgMap.insert(std::make_pair(msg, id));
		return id;
	}
	GMessageID GMessage::GetMsgID(const GMessageStr& msg) {
		GMessage& m = Ref();
		auto itr = m._msgMap.find(msg);
		Assert(Trap, itr != m._msgMap.end())
		return itr->second;
	}

	// -------------------- UpdBase --------------------
	UpdBase::UpdBase() {}
	UpdBase::UpdBase(Priority prio): _priority(prio) {}
	bool GOBase::onUpdateUpd() {
		if(isDead())
			return true;
		onUpdate();
		return isDead();
	}
	Priority UpdBase::getPriority() const { return _priority; }
	UpdChild* UpdBase::getParent() const { return _parent; }
	void UpdBase::setParent(UpdChild* uc) {
		_parent = uc;
	}
	// -------------------- UpdProxy --------------------
	UpdProxy::UpdProxy(Priority prio, HGbj hGbj): UpdBase(prio), _hlGbj(hGbj) {}
	bool UpdProxy::isNode() const { return false; }
	void UpdProxy::onDestroy() {
		_hlGbj.get().ref()->onDestroy();
	}
	void UpdProxy::proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) {
		p->updateProc(this);
	}
	void UpdProxy::proc(const IUpdProc* p) {
		p->updateProc(this);
	}
	UpdGroup* UpdProxy::findGroup(GroupID id) const {
		return nullptr;
	}
	UpdBase* UpdProxy::_clone() const {
		return new UpdProxy(getPriority(), _hlGbj.get());
	}
	void UpdProxy::onUpdate() {
		auto& p = _hlGbj.get().ref();
		p->onUpdate();
		if(p->isDead())
			destroy();
	}
	LCValue UpdProxy::recvMsg(GMessageID msg, const LCValue& arg) {
		return _hlGbj.get().ref()->recvMsg(msg, arg);
	}

	// -------------------- UpdChild --------------------
	UpdChild::UpdChild(const std::string& name): _name(name) {}
	UpdGroup* UpdChild::findGroup(GroupID id) const {
		if(_nGroup > 0) {
			for(auto* p : _child) {
				auto* res = p->findGroup(id);
				if(res)
					return res;
			}
		}
		return nullptr;
	}
	const std::string& UpdChild::getName() const { return _name; }
	void UpdChild::addObj(UpdBase* upd) {
		if(upd->isNode())
			++_nGroup;

		// 単純挿入ソート
		auto itr = _child.begin();
		Priority prio = upd->getPriority();
		while(itr != _child.end()) {
			if(prio < (*itr)->getPriority()) {
				_child.insert(itr, upd);
				return;
			}
			++itr;
		}
		_child.push_back(upd);
		upd->setParent(this);
	}
	void UpdChild::remObjs(const UpdArray& ar) {
		for(auto* p : ar) {
			auto itr = std::find(_child.begin(), _child.end(), p);
			if(itr != _child.end()) {
				if(p->isNode())
					--_nGroup;
				else
					p->onDestroy();
				delete p;
				_child.erase(itr);
			}
		}
	}
	const UpdList& UpdChild::getList() const { return _child; }
	UpdList& UpdChild::getList() { return _child; }
	void UpdChild::clear() {
		std::for_each(_child.begin(), _child.end(), [](UpdBase* p) { delete p; });
		_child.clear();
		_nGroup = 0;
	}

	// UpdChildは名前を付けて登録 = 参照カウントによる
	// Objectは名前を付けて登録, 上書き可, ハンドルの管理はしない
	// staticな名前をID登録, IDに対して値を割り当て
	// Obj[値は上書き可で、解放処理なし, 参照カウントなし, get時に有効化を判定して無効なら削除] = WHGBJ
	// UpdChild[値を上書き不可で、解放処理あり, entryがあれば有効] = UpdChild*
	// -------------------- UpdGroup --------------------
	UpdGroup::UGVec UpdGroup::s_ug;
	void UpdGroup::_doRemove() {
		_child.get().ref()->remObjs(_remObj);
		_remObj.clear();
	}
	UpdGroup::UpdGroup(Priority prio, HUpd hUpd): UpdBase(prio), _child(hUpd) {}
	UpdGroup::UpdGroup(Priority prio): _child(
		UpdMgr::_ref().acquire(UPUpdCh(new UpdChild()))
	) {}
	void UpdGroup::clear() {
		_child.get().ref()->clear();
	}
	void UpdGroup::addObj(Priority prio, HGbj hGbj) {
		addObj(new UpdProxy(prio, hGbj));
	}
	void UpdGroup::addObj(UpdBase* upd) {
		_child.get().ref()->addObj(upd);
	}
	void UpdGroup::remObj(UpdBase* upd) {
		// 後でonUpdateの時に削除する
		if(std::find(_remObj.begin(), _remObj.end(), upd) == _remObj.end())
			_remObj.push_back(upd);
	}
	const std::string& UpdGroup::getGroupName() const { return _child.get().cref()->getName(); }
	void UpdGroup::setIdle(int nFrame) {
		_idleCount = nFrame;
	}
	int UpdGroup::getAccum() const { return _accum; }
	void UpdGroup::onUpdate() {
		// アイドル時間チェック
		if(_idleCount > 0)
			--_idleCount;
		else {
			for(auto ent : _child.get().ref()->getList()) {
				auto b = ent->onUpdateUpd();
				if(b) {
					// 次のフレーム直前で消す
					remObj(ent);
				}
			}
		}
		++_accum;
		// 何か削除するノードを持つグループを登録
		if(!_remObj.empty())
			s_ug.push_back(this);

		// ルートノードで一括してオブジェクトの削除
		if(!getParent() && !s_ug.empty()) {
			for(auto ent : s_ug)
				ent->_doRemove();
			s_ug.clear();
		}
	}
	UpdGroup* UpdGroup::findGroup(GroupID id) const {
		return _child.get().cref()->findGroup(id);
	}
	void UpdGroup::proc(Priority prioBegin, Priority prioEnd, const IUpdProc* p) {
		auto& ls = _child.get().ref()->getList();
		if(ls.empty())
			return;

		auto itr = ls.begin(),
			itrE = ls.end();
		// prBeginの優先度までスキップ
		for(;;) {
			if(itr == itrE)
				return;
			if((*itr)->getPriority() >= prioBegin)
				break;
			++itr;
		}
		// prEndに達したらそれ以上処理しない
		do {
			if(itr == itrE || (*itr)->getPriority() > prioEnd)
				return;
			(*itr)->proc(p);
			++itr;
		} while(itr != itrE);
	}
	void UpdGroup::proc(const IUpdProc* p) {
		for(auto ent : _child.get().ref()->getList())
			ent->proc(p);
	}
	LCValue UpdGroup::recvMsg(GMessageID msg, const LCValue& arg) {
		for(auto ent : _child.get().ref()->getList())
			ent->recvMsg(msg, arg);
		return LCValue();
	}
	HLUpd UpdGroup::clone() const {
		// UpdProxyは複製, UpdGroupは参照コピー
		auto& c = _child.get().cref()->getList();
		HLUpd hlUpd(UpdMgr::_ref().acquire(UPUpdCh(new UpdChild())));
		auto& nc = hlUpd.get().ref()->getList();
		for(auto* u : c)
			nc.push_back(u->_clone());
		return std::move(hlUpd);
	}
	UpdBase* UpdGroup::_clone() const {
		// Child参照をそのまま返す
		auto* p = new UpdGroup(getPriority(), _child.get());
		p->_idleCount = _idleCount;
		return p;
	}
	HUpd UpdGroup::getChild() const { return _child.get(); }
	bool UpdGroup::isNode() const { return true; }
}
