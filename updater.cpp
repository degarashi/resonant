#include "updater.hpp"
#include "spinner/sort.hpp"

ImplDrawGroup(rs::U_DrawGroup, 0x0000)
ImplDrawGroup(rs::U_DrawGroupProxy, 0x0000)
namespace rs {
	const ObjTypeId InvalidObjId(~0);
	HGroup ObjMgr::CastToGroup(HObj hObj) {
		return Cast<GroupUP>(hObj);
	}
	const std::string& ObjMgr::getResourceName(spn::SHandle sh) const {
		if(sh) {
			Object* obj = HObj::FromHandle(sh)->get();
			return obj->getName();
		}
		const static std::string name("Object");
		return name;
	}

	namespace {
		const std::string cs_objname("Object"),
						cs_updgroupname("UpdGroup"),
						cs_updtaskname("UpdTask"),
						cs_drawgroupname("DrawGroup");
	}
	const DSortSP cs_dsort_z_asc = std::make_shared<DSort_Z_Asc>(),
					cs_dsort_z_desc = std::make_shared<DSort_Z_Desc>(),
					cs_dsort_priority_asc = std::make_shared<DSort_Priority_Asc>(),
					cs_dsort_priority_desc = std::make_shared<DSort_Priority_Desc>(),
					cs_dsort_techpass = std::make_shared<DSort_TechPass>(),
					cs_dsort_texture = std::make_shared<DSort_Texture>(),
					cs_dsort_buffer = std::make_shared<DSort_Buffer>();
	// -------------------- DrawTag --------------------
	DrawTag::DrawTag() {
		zOffset = DSort_Z_Asc::cs_border;
		priority = 0;
		idTechPass.value = DSort_TechPass::cs_invalidValue;
	}
	DrawTag::TPId::TPId(): preId() {}
	DrawTag::TPId& DrawTag::TPId::operator = (const GL16Id& id) {
		value = 0x80000000 | (id[0] << sizeof(id[0])) | id[1];
		return *this;
	}
	DrawTag::TPId& DrawTag::TPId::operator = (const IdValue& id) {
		value = id.value;
		return *this;
	}
	// -------------------- Object --------------------
	Object::Object(): _bDestroy(false) {}
	Priority Object::getPriority() const {
		return 0;
	}
	bool Object::isDead() const {
		return _bDestroy;
	}
	bool Object::onUpdateUpd(const SPLua& ls) {
		if(isDead())
			return true;
		onUpdate(ls, true);
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
	void Object::onUpdate(const SPLua& /*ls*/, bool /*bFirst*/) {}
	void Object::destroy() {
		_bDestroy = true;
	}
	const std::string& Object::getName() const {
		return cs_objname;
	}
	void Object::enumGroup(CBFindGroup /*cb*/, GroupTypeId /*id*/, int /*depth*/) const {
		Assert(Warn, "not supported operation")
	}
	LCValue Object::recvMsg(const GMessageStr& /*id*/, const LCValue& /*arg*/) {
		return LCValue();
	}
	LCValue Object::recvMsgLua(const SPLua& /*ls*/, const GMessageStr& /*id*/, const LCValue& /*arg*/) {
		return LCValue();
	}
	void Object::proc(UpdProc /*p*/, bool /*bRecursive*/, Priority /*prioBegin*/, Priority /*prioEnd*/) {
		Assert(Warn, "not supported operation")
	}
	void Object::onDraw(IEffect& /*e*/) const {}
	void Object::onDown(ObjTypeId /*prevId*/, const LCValue& /*arg*/) {}
	void Object::onPause() {}
	void Object::onStop() {}
	void Object::onResume() {}
	void Object::onReStart() {}
	// -------------------- IScene --------------------
	HGroup IScene::getUpdGroup() const {
		AssertP(Warn, false, "invalid function call")
		return HGroup();
	}
	HDGroup IScene::getDrawGroup() const {
		AssertP(Warn, false, "invalid function call")
		return HDGroup();
	}
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
	void UpdGroup::addObjPriority(HObj hObj, Priority p) {
		_registerUGVec();
		AssertP(Trap, std::count_if(_addObj.begin(), _addObj.end(),
					[hObj](const auto& ao){ return ao.second.get() == hObj; }) == 0, "オブジェクトの重複登録")
		// すぐ追加するとリスト巡回が不具合起こすので後で一括処理
		_addObj.emplace_back(p, hObj);
	}
	void UpdGroup::addObj(HObj hObj) {
		addObjPriority(hObj, hObj->get()->getPriority());
	}
	void UpdGroup::_registerUGVec() {
		if(_addObj.empty() && _remObj.empty())
			s_ug.push_back(this);
	}
	void UpdGroup::remObj(HObj hObj) {
		_registerUGVec();
		// すぐ削除するとリスト巡回が不具合起こすので後で一括削除
		// onUpdateの最後で削除する
		AssertP(Trap, std::find(_remObj.begin(), _remObj.end(), hObj) == _remObj.end(), "同一オブジェクトの複数回削除")
		_remObj.emplace_back(hObj);
	}
	const UpdGroup::ObjVP& UpdGroup::getList() const {
		return _objV;
	}
	UpdGroup::ObjVP& UpdGroup::getList() {
		return _objV;
	}
	void UpdGroup::clear() {
		_addObj.clear();
		_remObj.clear();
		// Remove時の処理をする為、一旦RemoveListに追加
		for(auto& obj : _objV)
			_remObj.push_back(obj.second);
		std::copy(_groupV.begin(), _groupV.end(), std::back_inserter(_remObj));
		_doAddRemove();

		AssertP(Trap, _objV.empty() && _groupV.empty() && _addObj.empty() && _remObj.empty())
	}
	void UpdGroup::onDraw(IEffect& e) const {
		// DrawUpdate中のオブジェクト追加削除はナシ
		for(auto& obj : _objV)
			obj.second->get()->onDraw(e);
	}
	void UpdGroup::onUpdate(const SPLua& ls, bool /*bFirst*/) {
		{
			class FlagSet {
				private:
					bool _bRootPrev = tls_bUpdateRoot;
				public:
					FlagSet(): _bRootPrev(tls_bUpdateRoot) {
						tls_bUpdateRoot = false; }
					~FlagSet() {
						tls_bUpdateRoot = _bRootPrev; }
			} flagset;

			for(auto& obj : _objV) {
				auto* ent = obj.second->get();
				auto b = ent->onUpdateUpd(ls);
				if(b) {
					// 次のフレーム直前で消す
					remObj(obj.second.get());
				}
			}
		}
		// ルートノードで一括してオブジェクトの追加、削除
		if(tls_bUpdateRoot) {
			while(!s_ug.empty()) {
				// 削除中、他に追加削除されるオブジェクトが出るかも知れないので一旦リストを退避
				decltype(s_ug) tmp;
				tmp.swap(s_ug);
				for(auto ent : tmp)
					ent->_doAddRemove();
			}
		}
	}
	UpdGroup::~UpdGroup() {
		if(!std::uncaught_exception()) {
			AssertP(Trap, _nParent==0)
		}
	}
	void UpdGroup::onConnected(HGroup /*hGroup*/) {
		++_nParent;
	}
	void UpdGroup::onDisconnected(HGroup /*hGroup*/) {
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
			if(itr->first >= prioBegin)
				break;
			++itr;
		}
		// prEndに達したらそれ以上処理しない
		do {
			if(itr == itrE || itr->first > prioEnd)
				return;
			itr->second->get()->proc(p, bRecursive);
			++itr;
		} while(itr != itrE);
	}
	LCValue UpdGroup::recvMsg(const GMessageStr& msg, const LCValue& arg) {
		for(auto& obj : _objV)
			obj.second->get()->recvMsg(msg, arg);
		return LCValue();
	}
	LCValue UpdGroup::recvMsgLua(const SPLua& ls, const GMessageStr& msg, const LCValue& arg) {
		for(auto& obj : _objV)
			obj.second->get()->recvMsgLua(ls, msg, arg);
		return LCValue();
	}
	UpdGroup::UGVec UpdGroup::s_ug;
	void UpdGroup::_doAddRemove() {
		auto hThis = HGroup::FromHandle(handleFromThis());
		for(;;) {
			// -- add --
			// オブジェクト追加中に更に追加オブジェクトが出るかも知れないので一旦退避
			auto addTmp = std::move(_addObj);
			for(auto& h : addTmp) {
				auto* p = h.second->get();
				if(p->isNode()) {
					// Groupリスト自体はソートしない
					_groupV.emplace_back(rs_mgr_obj.CastToGroup(h.second));
				}
				// 末尾に追加
				_objV.emplace_back(h);
				p->onConnected(hThis);
			}
			// -- remove --
			// オブジェクト削除中に更に削除オブジェクトが出るかも知れないので一旦退避
			auto remTmp = std::move(_remObj);
			for(auto& h : remTmp) {
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
				auto itr = std::find_if(_objV.begin(), _objV.end(), [&h](const auto& obj){
											return obj.second.get() == h;
										});
				if(itr != _objV.end()) {
					p->onDisconnected(hThis);
					_objV.erase(itr);
				}
			}
			if(_addObj.empty() && _remObj.empty()) {
				// 優先度値は変化しないので多分、単純挿入ソートが最適
				spn::insertion_sort(_objV.begin(), _objV.end(), [](const auto& obj0, const auto& obj1){
						return obj0.first < obj1.first;});
				break;
			}
		}
	}
	// -------------------- UpdTask --------------------
	UpdTask::UpdTask(Priority /*p*/, HGroup hGroup):
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
	LCValue UpdTask::recvMsg(const GMessageStr& msg, const LCValue& arg) {
		return _hlGroup->get()->recvMsg(msg, arg);
	}
	LCValue UpdTask::recvMsgLua(const SPLua& ls, const GMessageStr& msg, const LCValue& arg) {
		return _hlGroup->get()->recvMsgLua(ls, msg, arg);
	}

	const std::string& UpdTask::getName() const {
		return cs_updtaskname;
	}
	void UpdTask::onUpdate(const SPLua& ls, bool /*bFirst*/) {
		// アイドル時間チェック
		if(_idleCount > 0)
			--_idleCount;
		else
			_hlGroup->get()->onUpdate(ls, true);
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
	const DSortSP DrawGroup::cs_dsort[static_cast<int>(SortAlg::_Num)] = {
		cs_dsort_z_asc,
		cs_dsort_z_desc,
		cs_dsort_priority_asc,
		cs_dsort_priority_desc,
		cs_dsort_techpass,
		cs_dsort_texture,
		cs_dsort_buffer
	};
	DSortV DrawGroup::_MakeDSort(const SortAlgList& al) {
		auto sz = al.size();
		DSortV ret(sz);
		for(decltype(sz) i=0 ; i<sz ; i++) {
			ret[i] = cs_dsort[static_cast<int>(al[i])];
		}
		return ret;
	}
	DrawGroup::DrawGroup(): DrawGroup(DSortV{}) {}
	DrawGroup::DrawGroup(const DSortV& ds, bool bDynamic):
		_dsort(ds),
		_bDynamic(bDynamic)
	{}
	DrawGroup::DrawGroup(const SortAlgList& al, bool bDynamic):
		_dsort(_MakeDSort(al)),
		_bDynamic(bDynamic)
	{}
	void DrawGroup::setPriority(Priority p) {
		_dtag.priority = p;
	}
	bool DrawGroup::isNode() const {
		return true;
	}
	void DrawGroup::setSortAlgorithmId(const SortAlgList& al, bool bDynamic) {
		setSortAlgorithm(_MakeDSort(al), bDynamic);
	}
	void DrawGroup::setSortAlgorithm(const DSortV& ds, bool bDynamic) {
		_dsort = ds;
		_bDynamic = bDynamic;
		if(!bDynamic) {
			// リスト中のオブジェクトをソートし直す
			_doDrawSort();
		}
	}
	const DSortV& DrawGroup::getSortAlgorithm() const {
		return _dsort;
	}
	const DLObjV& DrawGroup::getMember() const {
		return _dobj;
	}
	DrawTag& DrawGroup::refDTag() {
		return _dtag;
	}
	void DrawGroup::addObj(HDObj hObj) {
		auto* dtag = &hObj->get()->getDTag();
		_dobj.emplace_back(dtag, hObj);
		// 毎フレームソートする設定でない時はここでソートする
		if(!_bDynamic)
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
	void DrawGroup::onUpdate(const SPLua& /*ls*/, bool /*bFirst*/) {
		Assert(Warn, "called deleted function: DrawGroup::onUpdate()")
	}
	void DrawGroup::onDraw(IEffect& e) const {
		if(_bDynamic) {
			// 微妙な実装
			const_cast<DrawGroup*>(this)->_doDrawSort();
		}
		// ソート済みの描画オブジェクトを1つずつ処理していく
		for(auto& d : _dobj) {
			d.second->get()->onDraw(e);
		}
	}

	// -------------------- DrawGroupProxy --------------------
	DrawGroupProxy::DrawGroupProxy(HDGroup hDg): _hlDGroup(hDg) {}
	void DrawGroupProxy::onUpdate(const SPLua& ls, bool bFirst) {
		// DrawGroupのonUpdateを呼ぶとエラーになるが、一応呼び出し
		_hlDGroup->get()->onUpdate(ls, bFirst);
	}
	void DrawGroupProxy::setPriority(Priority p) {
		_dtag.priority = p;
	}
	const DSortV& DrawGroupProxy::getSortAlgorithm() const {
		return _hlDGroup->get()->getSortAlgorithm();
	}
	const DLObjV& DrawGroupProxy::getMember() const {
		return _hlDGroup->get()->getMember();
	}
	bool DrawGroupProxy::isNode() const {
		return true;
	}
	void DrawGroupProxy::onDraw(IEffect& e) const {
		_hlDGroup->get()->onDraw(e);
	}
	const std::string& DrawGroupProxy::getName() const {
		return _hlDGroup->get()->getName();
	}
	DrawTag& DrawGroupProxy::refDTag() {
		return _dtag;
	}
	// -------------------- ObjectT_LuaBase --------------------
	LCValue detail::ObjectT_LuaBase::CallRecvMsg(const SPLua& ls, HObj hObj, const GMessageStr& msg, const LCValue& arg) {
		ls->push(hObj);
		LValueS lv(ls->getLS());
		LCValue ret = lv.callMethodNRet(luaNS::RecvMsg, msg, arg);
		auto& tbl = *boost::get<SPLCTable>(ret);
		if(tbl[1]) {
			int sz = tbl.size();
			for(int i=1 ; i<=sz ; i++)
				tbl[i] = std::move(tbl[i+1]);
			tbl.erase(tbl.find(sz));
			return ret;
		}
		return LCValue();
	}
}
