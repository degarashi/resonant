#include "updater.hpp"
#include "spinner/sort.hpp"

namespace rs {
	const ObjTypeId InvalidObjId(~0);
	namespace {
		const std::string cs_objname("Object"),
						cs_updgroupname("UpdGroup"),
						cs_updtaskname("UpdTask"),
						cs_drawgroupname("DrawGroup");
	}
	const DSortSP cs_dsort_z_asc = std::make_shared<DSort_Z_Asc>(),
					cs_dsort_z_desc = std::make_shared<DSort_Z_Desc>(),
					cs_dsort_techpass = std::make_shared<DSort_TechPass>(),
					cs_dsort_texture = std::make_shared<DSort_Texture>(),
					cs_dsort_buffer = std::make_shared<DSort_Buffer>();

	// -------------------- Object --------------------
	Object::Object(): _bDestroy(false) {}
	void Object::_initState() {
		initState();
		_doSwitchState();
	}
	void Object::_doSwitchState() {}
	void Object::initState() {}
	Priority Object::getPriority() const {
		return 0;
	}

	bool Object::isDead() const {
		return _bDestroy;
	}
	bool Object::onUpdateUpd() {
		if(isDead())
			return true;
		onUpdate();
		return isDead();
	}
	void* Object::getInterface(InterfaceId /*id*/) {
		return nullptr;
	}
	const void* Object::getInterface(InterfaceId id) const {
		return const_cast<Object*>(this)->getInterface(id);
	}
	void Object::onConnected(HGroup /*hGroup*/) {}
	void Object::onDisconnected(HGroup /*hGroup*/) {}
	void Object::onUpdate() {}
	void Object::destroy() {
		_bDestroy = true;
	}
	const std::string& Object::getName() const {
		return cs_objname;
	}
	Object::Form Object::getForm() const {
		return Form::Invalid;
	}
	void Object::onHitEnter(HObj /*hObj*/) {}				//!< 初回衝突
	void Object::onHit(HObj /*hObj*/, int /*n*/) {}			//!< 2フレーム目以降
	void Object::onHitExit(WObj /*whObj*/, int /*n*/) {}

	void Object::enumGroup(CBFindGroup /*cb*/, GroupTypeId /*id*/, int /*depth*/) const {
		Assert(Warn, "not supported operation")
	}
	LCValue Object::recvMsg(GMessageId /*id*/, const LCValue& /*arg*/) {
		return LCValue();
	}
	void Object::proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) {
		Assert(Warn, "not supported operation")
	}
	void Object::onDraw() const {}
	void Object::onDown(ObjTypeId /*prevId*/, const LCValue& /*arg*/) {}
	void Object::onPause() {}
	void Object::onStop() {}
	void Object::onResume() {}
	void Object::onReStart() {}

	// -------------------- UpdGroup --------------------
	UpdGroup::UpdGroup(Priority p):
		_priority(p),
		_nParent(0)
	{}
	thread_local bool UpdGroup::tls_bUpdateRoot = false;
	void UpdGroup::SetAsUpdateRoot() {
		tls_bUpdateRoot = true;
	}
	Priority UpdGroup::getPriority() const {
		return _priority;
	}
	bool UpdGroup::isNode() const {
		return true;
	}
	void UpdGroup::enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const {
		for(auto& h : _groupV) {
			if(h->get()->getTypeId() == id)
				cb(h);
		}
		if(depth > 0) {
			--depth;
			for(auto& h : _groupV) {
				h->get()->enumGroup(cb, id, depth);
			}
		}
	}
	const std::string& UpdGroup::getName() const {
		return cs_updgroupname;
	}
	void UpdGroup::addObj(HObj hObj) {
		auto* p = hObj->get();
		if(p->isNode()) {
			// Groupリスト自体はソートしない
			_groupV.emplace_back(rs_mgr_obj.CastToGroup(hObj));
		}

		// 末尾に追加してソートをかける
		_objV.emplace_back(hObj);
		// 優先度値は変化しないので多分、単純挿入ソートが最適
		spn::insertion_sort(_objV.begin(), _objV.end(), [](const HLObj& hl0, const HLObj& hl1){
				return hl0->get()->getPriority() < hl1->get()->getPriority(); });

		auto h = handleFromThis();
		p->onConnected(rs_mgr_obj.CastToGroup(h.get()));
	}
	int UpdGroup::remObj(const ObjVH& ar) {
		if(_remObj.empty())
			s_ug.push_back(this);
		int count = 0;
		for(auto& h : ar) {
			// すぐ削除するとリスト巡回が不具合起こすので後で一括削除
			// 後でonUpdateの時に削除する
			if(std::find(_remObj.begin(), _remObj.end(), h) == _remObj.end()) {
				_remObj.emplace_back(h);
				++count;
			}
		}
		return count;
	}
	void UpdGroup::remObj(HObj hObj) {
		remObj(ObjVH{hObj});
	}
	const UpdGroup::ObjV& UpdGroup::getList() const {
		return _objV;
	}
	UpdGroup::ObjV& UpdGroup::getList() {
		return _objV;
	}
	void UpdGroup::clear() {
		_remObj.clear();
		std::copy(_objV.begin(), _objV.end(), std::back_inserter(_remObj));
		std::copy(_groupV.begin(), _groupV.end(), std::back_inserter(_remObj));
		_doRemove();

		AssertP(Trap, _objV.empty() && _groupV.empty() && _remObj.empty())
	}
	void UpdGroup::onDraw() const {
		// DrawUpdate中のオブジェクト追加削除はナシ
		for(auto& h : _objV)
			h->get()->onDraw();
	}
	void UpdGroup::onUpdate() {
		struct FlagSet {
			bool bRootPrev = tls_bUpdateRoot;
			~FlagSet() { tls_bUpdateRoot = bRootPrev; }
		} flagset;

		for(auto& h : _objV) {
			auto* ent = h->get();
			auto b = ent->onUpdateUpd();
			if(b) {
				// 次のフレーム直前で消す
				remObj(h.get());
			}
		}
		// ルートノードで一括してオブジェクトの削除
		if(tls_bUpdateRoot) {
			while(!s_ug.empty()) {
				decltype(s_ug) tmp;
				tmp.swap(s_ug);
				for(auto ent : tmp)
					ent->_doRemove();
			}
		}
	}
	UpdGroup::~UpdGroup() {
		AssertP(Trap, _nParent==0)
	}
	void UpdGroup::onConnected(HGroup hGroup) {
		++_nParent;
	}
	void UpdGroup::onDisconnected(HGroup hGroup) {
		// 親グループから切り離された数のチェック
		--_nParent;
		AssertP(Trap, _nParent >= 0)
		if(_nParent == 0)
			clear();
	}
	void UpdGroup::proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) {
		if(_objV.empty())
			return;

		auto itr = _objV.begin(),
			itrE = _objV.end();
		// prBeginの優先度までスキップ
		for(;;) {
			if(itr == itrE)
				return;
			if((*itr)->get()->getPriority() >= prioBegin)
				break;
			++itr;
		}
		// prEndに達したらそれ以上処理しない
		do {
			if(itr == itrE || (*itr)->get()->getPriority() > prioEnd)
				return;
			(*itr)->get()->proc(p, bRecursive);
			++itr;
		} while(itr != itrE);
	}
	LCValue UpdGroup::recvMsg(GMessageId msg, const LCValue& arg) {
		for(auto& h : _objV)
			h->get()->recvMsg(msg, arg);
		return LCValue();
	}
	UpdGroup::UGVec UpdGroup::s_ug;
	void UpdGroup::_doRemove() {
		auto hThis = handleFromThis();
		for(auto& h : _remObj) {
			auto* p = h->get();
			if(p->isNode()) {
				auto itr = std::find_if(_groupV.begin(), _groupV.end(), [&h](const HLGroup& hl){
											return hl.get() == h;
										});
				if(itr != _groupV.end())
					_groupV.erase(itr);
				else
					continue;
			}
			auto itr = std::find_if(_objV.begin(), _objV.end(), [&h](const HLObj& hl){
										return hl.get() == h;
									});
			if(itr != _objV.end()) {
				p->onDisconnected(hThis);
				_objV.erase(itr);
			}
		}
		_remObj.clear();
	}
	// -------------------- UpdTask --------------------
	UpdTask::UpdTask(Priority p, HGroup hGroup):
		_idleCount(0), _accum(0),
		_hlGroup(hGroup)
	{}
	bool UpdTask::isNode() const {
		return true;
	}
	void UpdTask::onConnected(HGroup hGroup) {
		_hlGroup->get()->onConnected(hGroup);
	}
	void UpdTask::onDisconnected(HGroup hGroup) {
		_hlGroup->get()->onDisconnected(hGroup);
	}
	void UpdTask::enumGroup(CBFindGroup cb, GroupTypeId id, int depth) const {
		_hlGroup->get()->enumGroup(cb, id, depth);
	}
	void UpdTask::proc(UpdProc p, bool bRecursive, Priority prioBegin, Priority prioEnd) {
		_hlGroup->get()->proc(p, bRecursive, prioBegin, prioEnd);
	}
	LCValue UpdTask::recvMsg(GMessageId msg, const LCValue& arg) {
		return _hlGroup->get()->recvMsg(msg, arg);
	}

	const std::string& UpdTask::getName() const {
		return cs_updtaskname;
	}
	void UpdTask::onUpdate() {
		// アイドル時間チェック
		if(_idleCount > 0)
			--_idleCount;
		else
			_hlGroup->get()->onUpdate();
		++_accum;
	}
	void UpdTask::setIdle(int nFrame) {
		_idleCount = nFrame;
	}
	int UpdTask::getAccum() const {
		return _accum;
	}

	// -------------------- DrawableObj --------------------
	const DrawTag& DrawableObj::getDTag() const {
		return _dtag;
	}
	// -------------------- DrawGroup --------------------
	DrawGroup::DrawGroup(const DSortV& ds, bool bSort):
		_dsort(ds), _bSort(bSort) {}
	bool DrawGroup::isNode() const {
		// DrawGroupの下に別のGroupは含めない
		return false;
	}
	void DrawGroup::addObj(HDObj hObj) {
		auto* pObj = hObj->get();
		if(pObj->isNode()) {
			Assert(Warn, false, "DrawGroup can't have any groups")
			return;
		}

		auto* dtag = &pObj->getDTag();
		_dobj.emplace_back(dtag, hObj);
		// 毎フレームソートする設定でない時はここでソートする
		if(!_bSort)
			_doDrawSort();
	}
	void DrawGroup::remObj(HDObj hObj) {
		auto itr = std::find_if(_dobj.begin(), _dobj.end(), [hObj](const auto& p){
			return p.second.get() == hObj;
		});
		if(itr == _dobj.end()) {
			Assert(Warn, "object not found")
			return;
		}
		_dobj.erase(itr);
	}
	void DrawGroup::_doDrawSort() {
		DSort::DoSort(_dsort, 0, _dobj.begin(), _dobj.end());
	}
	const std::string& DrawGroup::getName() const {
		return cs_drawgroupname;
	}
	void DrawGroup::onUpdate() {
		Assert(Warn, "called deleted function: DrawGroup::onUpdate()")
	}
	void DrawGroup::onDraw() const {
		if(_bSort) {
			// 微妙な実装
			const_cast<DrawGroup*>(this)->_doDrawSort();
		}
		// ソート済みの描画オブジェクトを1つずつ処理していく
		for(auto& d : _dobj) {
			d.second->get()->onDraw();
		}
	}
}
