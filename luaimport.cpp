#include "luaw.hpp"
#include "updater_lua.hpp"
#include <boost/preprocessor.hpp>
#include <boost/regex.hpp>
#include "sdlwrap.hpp"
#include <cstring>
#include "scene.hpp"
#include "apppath.hpp"

/*	Luaハンドル仕様：
	object = {
		udata = (Userdata)
		...(ユーザーのデータ色々)
	}
*/
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, U_Scene, NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR(SceneMgr, NOTHING, (isEmpty)(getTop), NOTHING)
namespace rs {
	using spn::SHandle;
	using spn::WHandle;
	namespace luaNS {
		const std::string ScriptResourceEntry("script"),
								SystemScriptResourceEntry("system_script"),
								ScriptExtension("lua");
		const std::string Udata("udata"),
						Pointer("pointer"),
						ToString("tostring");
		const std::string GetHandle("GetHandle"),
						DeleteHandle("DeleteHandle"),
						ObjectBase("ObjectBase"),
						DerivedHandle("DerivedHandle"),
						MakeFSMachine("MakeFSMachine"),
						MakePreENV("MakePreENV"),
						RecvMsg("RecvMsg"),
						System("System");
		namespace objBase {
			const std::string ValueR("_valueR"),
								ValueW("_valueW"),
								Func("_func"),
								UdataMT("_udata_mt"),
								MT("_mt"),
								_New("_New");
			namespace valueR {
				const std::string HandleId("handleId"),
									NumRef("numRef");
			}
		}
		namespace system {
			const std::string PathSeparation(";"),
								PathReplaceMark("?"),
								Package("package"),
								Path("path");
		}
	}
	namespace {
		// Userdata(Handle)をスタックの一番上に積んだ上で呼ぶ
		int IncrementHandle(lua_State* ls) {
			SHandle sh = LCV<SHandle>()(1, ls);
			spn::ResMgrBase::Increment(sh);
			return 0;
		}
		int DecrementHandle(lua_State* ls) {
			SHandle sh = LCV<SHandle>()(1, ls);
			sh.release();
			return 0;
		}
	}
	lua_Unsigned LuaImport::HandleId(SHandle sh) {
		return sh.getValue(); }
	lua_Integer LuaImport::NumRef(SHandle sh) {
		return sh.count(); }

	void* LI_GetPtr::operator()(lua_State* ls, int idx) const {
		lua_getfield(ls, idx, luaNS::Pointer.c_str());
		void* ret = LCV<void*>()(-1, ls);
		lua_pop(ls, 1);
		return ret;
	}
	void* LI_GetHandleBase::operator()(lua_State* ls, int idx) const {
		SHandle sh = getHandle(ls, idx);
		return spn::ResMgrBase::GetPtr(sh);
	}
	SHandle LI_GetHandleBase::getHandle(lua_State* ls, int idx) const {
		lua_getfield(ls, idx, luaNS::Udata.c_str());
		SHandle ret(reinterpret_cast<uintptr_t>(LCV<void*>()(-1, ls)));
		lua_pop(ls, 1);
		return ret;
	}
	bool LuaImport::IsObjectBaseRegistered(LuaState& lsc) {
		lsc.getGlobal(luaNS::ObjectBase);
		bool res = lsc.type(-1) == LuaType::Table;
		lsc.pop();
		return res;
	}
	// オブジェクトハンドルの基本メソッド定義
	void LuaImport::RegisterObjectBase(LuaState& lsc) {
		if(IsObjectBaseRegistered(lsc))
			return;

		lsc.newTable();

		// ValueRの初期化
		// ValueR = { HandleId=(HandleId), NumRef=(NumRef) }
		lsc.push(luaNS::objBase::ValueR);
		lsc.newTable();
		LuaImport::RegisterMember<LI_GetHandle<SHandle>, SHandle>(lsc, luaNS::objBase::valueR::HandleId.c_str(), &SHandle::getValue);
		lsc.push(luaNS::objBase::valueR::NumRef);
		LuaImport::PushFunction(lsc, &NumRef);
		lsc.setTable(-3);
		lsc.setTable(-3);
		// ValueWの初期化
		// ValueW = {}
		lsc.push(luaNS::objBase::ValueW);
		lsc.newTable();
		lsc.setTable(-3);
		// Funcの初期化
		// Func = {}
		lsc.push(luaNS::objBase::Func);
		lsc.newTable();
		lsc.setTable(-3);

		// C++リソースハンドル用メタテーブル
		// UdataMT = { __gc = (DecrementHandle) }
		lsc.push(luaNS::objBase::UdataMT);
		lsc.newTable();
		lsc.push("__gc");
		lsc.pushCClosure(DecrementHandle, 0);
		lsc.setTable(-3);
		lsc.setTable(-3);

		// IncrementHandle = (IncrementHandle)
		lsc.pushCClosure(IncrementHandle, 0);
		lsc.setGlobal("IncrementHandle");
		// DecrementHandle = (DecrementHandle)
		lsc.pushCClosure(DecrementHandle, 0);
		lsc.setGlobal("DecrementHandle");

		lsc.push(luaNS::RecvMsg);
		lsc.pushCClosure(LuaImport::RecvMsg, 0);
		lsc.setTable(-3);
		// ObjectBase = {...}
		lsc.setGlobal(luaNS::ObjectBase);

		std::string fileName("base." + luaNS::ScriptExtension);
		HLRW hlRW = mgr_path.getRW(luaNS::SystemScriptResourceEntry, fileName, nullptr);
		Assert(Trap, hlRW, "system script file \"%1%\" not found.", fileName)
		lsc.loadFromSource(hlRW, fileName.c_str(), true);
	}

	void LuaImport::LoadClass(LuaState& lsc, const std::string& name, HRW hRW) {
		RegisterObjectBase(lsc);

		lsc.newTable();
		lsc.getGlobal(luaNS::MakePreENV);
		lsc.pushValue(-2);
		lsc.call(1,0);
		// スタックに積むが、まだ実行はしない
		lsc.load(hRW, "LuaImport::LoadClass", "bt", false);
		// _ENVをクラステーブルに置き換えてからチャンクを実行
		// グローバル変数に代入しようとしたらクラスのstatic変数として扱う
		lsc.pushValue(-2);
		// [NewClassSrc][Chunk][NewClassSrc]
		lsc.setUpvalue(-2, 1);
		// [NewClassSrc][Chunk]
		lsc.call(0,0);
		// [NewClassSrc]
		lsc.getGlobal(luaNS::MakeFSMachine);
		lsc.pushValue(1);
		// [NewClassSrc][Func(DerivedClass)][NewClassSrc]
		lsc.call(1,1);
		lsc.setGlobal(name);
		lsc.pop(1);
		return;
	}
	int LuaImport::RecvMsg(lua_State* ls) {
		Object* obj = LI_GetHandle<typename ObjMgr::data_type>()(ls, 1);
		auto msgId = GMessage::GetMsgId(LCV<std::string>()(2, ls));
		if(msgId) {
			// Noneで無ければ有効な戻り値とする
			LCValue lcv = obj->recvMsg(*msgId, LCV<LCValue>()(3, ls));
			if(lcv.type() != LuaType::LNone) {
				lcv.push(ls);
				return 1;
			}
		}
		return 0;
	}

	// ------------------- LCV<SHandle> -------------------
	void LCV<SHandle>::operator()(lua_State* ls, SHandle h) const {
		// nullハンドルチェック
		if(!h.valid()) {
			// nilをpushする
			LCV<LuaNil>()(ls, LuaNil());
		} else {
			// Luaのハンドルテーブルからオブジェクトの実体を取得
			LuaState lsc(ls);
			lsc.getGlobal(luaNS::GetHandle);
			lsc.push(h.getValue());
			lsc.push(reinterpret_cast<void*>(h.getValue()));
			lsc.call(2,1);
			AssertP(Trap, lsc.type(-1) == LuaType::Table)
		}
	}
	SHandle LCV<SHandle>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		// userdata or nil
		LuaState lsc(ls);
		if(lsc.type(idx) == LuaType::Nil)
			return SHandle();
		return LI_GetHandle<SHandle>().getHandle(ls, -1);
	}
	std::ostream& LCV<SHandle>::operator()(std::ostream& os, SHandle h) const {
		return os << "(Handle)" << std::hex << "0x" << h.getValue();
	}
	LuaType LCV<SHandle>::operator()() const {
		return LuaType::Userdata;
	}
	// ------------------- LCV<WHandle> -------------------
	void LCV<WHandle>::operator()(lua_State* ls, WHandle w) const {
		if(!w.valid())
			LCV<LuaNil>()(ls, LuaNil());
		else {
			LuaState lsc(ls);
			lsc.push(reinterpret_cast<void*>(w.getValue()));
		}
	}
	WHandle LCV<WHandle>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		void* data = LCV<void*>()(idx, ls, spm);
		return WHandle(reinterpret_cast<WHandle::VWord>(data));
	}
	std::ostream& LCV<WHandle>::operator()(std::ostream& os, WHandle w) const {
		return os << "(WHandle)" << std::hex << "0x" << w.getValue();
	}
	LuaType LCV<WHandle>::operator()() const {
		return LuaType::LightUserdata;
	}

	//	アップデータの登録
	//		シーンツリーの管理はスクリプトがメイン
	//		HGroup = createGroup("")
	//		HGroup = loadGroup("")
	//		HGroup.add(0x123, hGroup)
	//		HGroup.rem(hGroup)
	//		Luaから
	//		C++から
}

