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
		const std::string GetInstance("GetInstance"),
						DeleteHandle("DeleteHandle"),
						SetState("SetState"),
						SwitchState("SwitchState"),
						Null("Null"),
						ObjectBase("ObjectBase"),
						ConstructPtr("ConstructPtr"),
						DerivedHandle("DerivedHandle"),
						MakeFSMachine("MakeFSMachine"),
						FSMachine("FSMachine"),
						MakePreENV("MakePreENV"),
						Ctor("Ctor"),
						RecvMsg("RecvMsg"),
						RecvMsgCpp("RecvMsgCpp"),
						OnUpdate("OnUpdate"),
						OnExit("OnExit"),
						System("System");
		namespace objBase {
			const std::string ValueR("_valueR"),
								ValueW("_valueW"),
								Func("_func"),
								UdataMT("_udata_mt"),
								MT("_mt"),
								ClassName("classname"),
								_Pointer("_pointer"),
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
			SHandle sh = GetLCVType<SHandle>()(1, ls);
			spn::ResMgrBase::Increment(sh);
			return 0;
		}
		int DecrementHandle(lua_State* ls) {
			SHandle sh = GetLCVType<SHandle>()(1, ls);
			int count = sh.count();
			sh.release();
			// リソースが削除される時はLua側の強参照も削除
			if(count == 1) {
				LuaState lsc(ls);
				lsc.getGlobal(luaNS::DeleteHandle);
				lsc.push(reinterpret_cast<void*>(sh.getValue()));
				lsc.call(1,0);
			}
			return 0;
		}
	}
	lua_Unsigned LuaImport::HandleId(SHandle sh) {
		return sh.getValue(); }
	lua_Integer LuaImport::NumRef(SHandle sh) {
		return sh.count(); }

	namespace {
		#ifdef DEBUG
			void CheckClassName(lua_State* ls, int idx, const char* name) {
				LuaState lsc(ls);
				lsc.getField(idx, luaNS::objBase::ClassName);
				auto objname = lsc.toString(-1);
				Assert(Trap, objname==name, "invalid object (required %1%, but got %2%)", name, objname)
				lsc.pop();
			}
		#else
			void CheckClassName(lua_State*, int, const char*) {}
		#endif
	}
	void* LI_GetPtrBase::operator()(lua_State* ls, int idx, const char* name) const {
		CheckClassName(ls, idx, name);
		LuaState lsc(ls);
		lsc.getField(idx, luaNS::Pointer);
		void* ret = LCV<void*>()(-1, ls);
		lsc.pop();
		return ret;
	}
	void* LI_GetHandleBase::operator()(lua_State* ls, int idx, const char* name) const {
		CheckClassName(ls, idx, name);
		SHandle sh = getHandle(ls, idx);
		return spn::ResMgrBase::GetPtr(sh);
	}
	SHandle LI_GetHandleBase::getHandle(lua_State* ls, int idx) const {
		LuaState lsc(ls);
		lsc.getField(idx, luaNS::Udata);
		SHandle ret(reinterpret_cast<uintptr_t>(LCV<void*>()(-1, ls)));
		lsc.pop();
		return ret;
	}
	bool LuaImport::IsObjectBaseRegistered(LuaState& lsc) {
		lsc.getGlobal(luaNS::ObjectBase);
		bool res = lsc.type(-1) == LuaType::Table;
		lsc.pop();
		return res;
	}
	int LuaImport::ReturnException(lua_State* ls, const char* func, const std::exception& e, int nNeed) {
		return luaL_error(ls, "Error occured at\nfunction: %s\ninput argument(s): %d\nneeded argument(s): %d\n"
								"---------------- error message ----------------\n%s\n"
								"-----------------------------------------------",
								func, lua_gettop(ls), nNeed, e.what());
	}
	namespace {
		int EmptyFunction(lua_State*) { return 0; }
	}
	namespace lua {
		template <>
		const char* LuaName(spn::SHandle*) {
			return "SHandle";
		}
		template <>
		const char* LuaName(IGLResource*) {
			return "IGLResource";
		}
	}
	// オブジェクト類を定義する為の基本関数定義など
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
		// RecvMsgCpp = func(RecvMsgCpp)
		lsc.push(luaNS::RecvMsgCpp);
		lsc.pushCClosure(LuaImport::RecvMsgCpp, 0);
		lsc.setTable(-3);
		// RecvMsg = func(RecvMsgCpp)
		lsc.push(luaNS::RecvMsg);
		lsc.pushCClosure(LuaImport::RecvMsgCpp, 0);
		lsc.setTable(-3);
		// Ctor = func(Ctor)
		lsc.push(luaNS::Ctor);
		lsc.pushCClosure(EmptyFunction, 0);
		lsc.setTable(-3);
		// global["ObjectBase"] = {...}
		lsc.setGlobal(luaNS::ObjectBase);

		// global["IncrementHandle"] = (IncrementHandle)
		lsc.pushCClosure(IncrementHandle, 0);
		lsc.setGlobal("IncrementHandle");
		// global["DecrementHandle"] = (DecrementHandle)
		lsc.pushCClosure(DecrementHandle, 0);
		lsc.setGlobal("DecrementHandle");

		lsc.loadModule("base");
	}
	bool LuaImport::IsUpdaterObjectRegistered(LuaState& lsc) {
		lsc.getGlobal(luaNS::FSMachine);
		bool res = lsc.type(-1) == LuaType::Table;
		lsc.pop();
		return res;
	}
	void LuaImport::RegisterUpdaterObject(LuaState& lsc) {
		if(IsUpdaterObjectRegistered(lsc))
			return;
		LuaImport::RegisterClass<Object>(lsc);

		std::string fileName("fsmachine." + luaNS::ScriptExtension);
		HLRW hlRW = mgr_path.getRW(luaNS::SystemScriptResourceEntry, fileName, nullptr);
		Assert(Trap, hlRW, "system script file \"%1%\" not found.", fileName)
		lsc.loadFromSource(hlRW, fileName.c_str(), true);
	}
	void LuaImport::LoadClass(LuaState& lsc, const std::string& name, HRW hRW) {
		RegisterObjectBase(lsc);
		RegisterUpdaterObject(lsc);

		// グローバルテーブルの付け替え
		lsc.newTable();
		lsc.getGlobal(luaNS::MakePreENV);
		lsc.pushValue(-2);
		lsc.call(1,0);
		// ユーザーのクラス定義をスタックに積むが、まだ実行はしない
		lsc.load(hRW, "LuaImport::LoadClass", "bt", false);
		// _ENVをクラステーブルに置き換えてからチャンクを実行
		// グローバル変数に代入しようとしたらクラスのstatic変数として扱う
		lsc.pushValue(-2);
		// [NewClassTable][UserChunk][NewClassTable]
		lsc.setUpvalue(-2, 1);
		// [NewClassTable][UserChunk]
		lsc.call(0,0);
		// [NewClassTable]
		lsc.getGlobal(luaNS::MakeFSMachine);
		lsc.pushValue(-2);
		lsc.push(name);
		// [NewClassTable][Func(MakeFSMachine)][ObjName][NewClassTable]
		lsc.call(2,1);
		lsc.setGlobal(name);
		lsc.pop(1);
		return;
	}
	void LuaImport::MakePointerInstance(LuaState& lsc, const std::string& luaName, void* ptr) {
		lsc.getGlobal(luaName);
		lsc.getField(-1, luaNS::ConstructPtr);
		lsc.push(ptr);
		lsc.call(1,1);
		// [ObjDefine][Instance]
		lsc.remove(-2);
	}
	int LuaImport::RecvMsgCpp(lua_State* ls) {
		Object* obj = LI_GetHandle<typename ObjMgr::data_type>()(ls, 1);
		auto msg = LCV<std::string>()(2, ls);
		// Noneで無ければ有効な戻り値とする
		LCValue lcv = obj->recvMsg(msg, LCV<LCValue>()(3, ls));
		if(lcv.type() != LuaType::LNone) {
			lua_pushboolean(ls, true);
			lcv.push(ls);
			return 2;
		}
		lua_pushboolean(ls, false);
		return 1;
	}
	LuaImport::LogMap LuaImport::s_logMap;
	std::stringstream LuaImport::s_importLog;
	std::string LuaImport::s_firstBlock;
	int LuaImport::s_indent = 0;
	void LuaImport::BeginImportBlock(const std::string& s) {
		if(s_indent > 0)
			_PushIndent(s_importLog) << '[' << s << ']' << std::endl;
		else
			s_firstBlock = s;
		++s_indent;
	}
	void LuaImport::EndImportBlock() {
		Assert(Trap, --s_indent >= 0, "indent error")
		if(s_indent == 0) {
			s_logMap[s_firstBlock].append(s_importLog.str());
			s_importLog.str("");
			s_importLog.clear();
		}
	}
	std::ostream& LuaImport::_PushIndent(std::ostream& s) {
		for(int i=0 ; i<s_indent ; i++)
			s << "    ";
		return s;
	}
	void LuaImport::SaveImportLog(const std::string& path) {
		std::ofstream ofs(path);
		Assert(Trap, ofs.is_open(), "can't open file %1%", path)
		for(auto& e : s_logMap) {
			ofs << '[' << e.first << ']' << std::endl << e.second;
		}
		s_logMap.clear();
	}

	// ------------------- LCV<SHandle> -------------------
	int LCV<SHandle>::operator()(lua_State* ls, SHandle h) const {
		// nullハンドルチェック
		if(!h.valid()) {
			// nilをpushする
			LCV<LuaNil>()(ls, LuaNil());
		} else {
			// Luaのハンドルテーブルからオブジェクトの実体を取得
			LuaState lsc(ls);
			auto& name = h.getResourceName();
			AssertP(Trap, !name.empty(), "invaild resource name.")
			lsc.getGlobal(name);
			lsc.getField(-1, luaNS::GetInstance);
			lsc.push(reinterpret_cast<void*>(h.getValue()));
			lsc.call(1,1);
			lsc.remove(-2);
			AssertP(Trap, lsc.type(-1) == LuaType::Table)
		}
		return 1;
	}
	SHandle LCV<SHandle>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		// userdata or nil
		LuaState lsc(ls);
		if(lsc.type(idx) == LuaType::Nil)
			return SHandle();
		return LI_GetHandle<SHandle>().getHandle(ls, idx);
	}
	std::ostream& LCV<SHandle>::operator()(std::ostream& os, SHandle h) const {
		return os << "(Handle)" << std::hex << "0x" << h.getValue();
	}
	LuaType LCV<SHandle>::operator()() const {
		return LuaType::Userdata;
	}
	DEF_LCV_OSTREAM(SHandle)
	// ------------------- LCV<WHandle> -------------------
	int LCV<WHandle>::operator()(lua_State* ls, WHandle w) const {
		if(!w.valid()) {
			LCV<LuaNil>()(ls, LuaNil());
			return 1;
		} else {
			LuaState lsc(ls);
			lsc.push(reinterpret_cast<void*>(w.getValue()));
			return LCV<SHandle>()(ls, spn::ResMgrBase::GetManager(w.getResID())->lock(w));
		}
	}
	WHandle LCV<WHandle>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		SHandle sh = LCV<SHandle>()(idx, ls, spm);
		if(sh)
			return sh.weak();
		return WHandle();
	}
	std::ostream& LCV<WHandle>::operator()(std::ostream& os, WHandle w) const {
		return os << "(WHandle)" << std::hex << "0x" << w.getValue();
	}
	LuaType LCV<WHandle>::operator()() const {
		return LuaType::LightUserdata;
	}
	DEF_LCV_OSTREAM(WHandle)
}
