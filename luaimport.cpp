#include "luaw.hpp"
#include "updator_lua.hpp"
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>
#include <boost/regex.hpp>
#include "sdlwrap.hpp"
#include <cstring>

/*	Luaハンドル仕様：
	object = {
		udata = (Userdata)
		...(ユーザーのデータ色々)
	}
*/
namespace rs {
	using spn::SHandle;
	namespace luaNS {
		const std::string Udata("udata");
		const std::string GetHandle("GetHandle"),
						DeleteHandle("DeleteHandle"),
						ObjectBase("ObjectBase"),
						DerivedHandle("DerivedHandle"),
						MakeStaticValueMT("MakeStaticValueMT"),
						MakeFSMachine("MakeFSMachine");
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

	void* LuaImport::GetPtr::operator()(lua_State* ls, int idx) const {
		return LCV<void*>()(idx, ls);
	}
	void* LuaImport::GetHandle::operator()(lua_State* ls, int idx) const {
		SHandle sh = getHandle(ls, idx);
		return spn::ResMgrBase::GetPtr(sh);
	}
	SHandle LuaImport::GetHandle::getHandle(lua_State* ls, int idx) const {
		lua_getfield(ls, idx, luaNS::Udata.c_str());
		SHandle ret(reinterpret_cast<uintptr_t>(LCV<void*>()(-1, ls)));
		lua_pop(ls, 1);
		return ret;
	}
	// オブジェクトハンドルの基本メソッド定義
	void LuaImport::RegisterObjectBase(LuaState& lsc) {
		lsc.newTable();

		// ValueRの初期化
		lsc.push(luaNS::objBase::ValueR);
		lsc.newTable();
		lsc.push(luaNS::objBase::valueR::HandleId);
		LuaImport::PushFunction(lsc, &HandleId);
		lsc.setTable(-3);
		lsc.push(luaNS::objBase::valueR::NumRef);
		LuaImport::PushFunction(lsc, &NumRef);
		lsc.setTable(-3);
		lsc.setTable(-3);
		// ValueWの初期化
		lsc.push(luaNS::objBase::ValueW);
		lsc.newTable();
		lsc.setTable(-3);
		// Funcの初期化
		lsc.newTable();
		lsc.push(luaNS::objBase::Func);
		lsc.setTable(-3);

		// C++リソースハンドル用メタテーブル
		lsc.push(luaNS::objBase::UdataMT);
		lsc.newTable();
		lsc.push("__gc");
		lsc.pushCClosure(DecrementHandle, 0);
		lsc.setTable(-3);
		lsc.setTable(-3);

		lsc.pushCClosure(IncrementHandle, 0);
		lsc.setGlobal("IncrementHandle");
		lsc.pushCClosure(DecrementHandle, 0);
		lsc.setGlobal("DecrementHandle");

		lsc.setGlobal(luaNS::ObjectBase);
		HLRW hlRW = mgr_rw.fromFile("/home/slice/projects/resonant/base.lua", RWops::Access::Read, true);
		lsc.loadFromSource(hlRW, "base.lua", true);
	}
	void LuaImport::LoadClass(LuaState& lsc, const std::string& name, HRW hRW) {
		lsc.newTable();
		lsc.getGlobal("MT_G");
		lsc.setMetatable(-1);
		// スタックに積むが、まだ実行はしない
		lsc.load(hRW, "LuaImport::LoadClass", "bt", false);
		// _ENVをクラステーブルに置き換えてからチャンクを実行
		// グローバル変数に代入しようとしたらクラスのstatic変数として扱う
		lsc.pushValue(-2);
		lsc.setUpvalue(-2, 1);
		// [NewClassSrc][Chunk]
		lsc.call(0,0);
		// クラステーブルの中の関数について、upvalueを独自のものに差し替える
		lsc.getGlobal(luaNS::MakeStaticValueMT);
		lsc.pushValue(1);
		lsc.call(1,1);
		// [NewClassSrc][StaticMT]
		lsc.push(LuaNil());
		while(lsc.next(1) != 0) {
			if(lsc.type(4) == LuaType::Function) {
				for(int i=1;;i++) {
					const char* c = lsc.getUpvalue(4, i);
					if(!c)
						break;
					if(!std::strcmp(c, "_ENV")) {
						lsc.pushValue(2);
						lsc.setUpvalue(4,i);
						lsc.pop(1);
						break;
					}
					lsc.pop(1);
				}
			}
			lsc.pop(1);
		}
		// [NewClassSrc]
		lsc.getGlobal(luaNS::MakeFSMachine);
		lsc.pushValue(1);
		// [NewClassSrc][Func(DerivedClass)][NewClassSrc]
		lsc.call(1,1);
		lsc.setGlobal(name);
		lsc.pop(2);
		return;
	}

	// ------------------- LCValue -------------------
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
	SHandle LCV<SHandle>::operator()(int idx, lua_State* ls) const {
		// userdata or nil
		LuaState lsc(ls);
		if(lsc.type(idx) == LuaType::Nil)
			return SHandle();
		return LuaImport::GetHandle().getHandle(ls, -1);
	}
	std::ostream& LCV<SHandle>::operator()(std::ostream& os, SHandle h) const {
		return os << "(Handle)" << std::hex << "0x" << h.getValue();
	}
	LuaType LCV<SHandle>::operator()() const {
		return LuaType::Userdata;
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

