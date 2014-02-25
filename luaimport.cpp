#include "luaw.hpp"
#include "updator_lua.hpp"
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>
#include <boost/regex.hpp>

namespace rs {
	using spn::SHandle;
	namespace luaNS {
		const std::string GetHandle("GetHandle"),
						DeleteHandle("DeleteHandle"),
						ObjectBase("ObjectBase"),
						DerivedHandle("DerivedHandle");
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
		void DecrementHandle(SHandle sh) {
			sh.release();
		}
	}
	lua_Unsigned LuaImport::HandleId(SHandle sh) {
		return sh.getValue(); }
	lua_Integer LuaImport::NumRef(SHandle sh) {
		return sh.count(); }

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
		LuaImport::PushFunction(lsc, DecrementHandle);
		lsc.setTable(-3);
		lsc.setTable(-3);

		lsc.setGlobal(luaNS::ObjectBase);
		lsc.load("/home/slice/projects/resonant/base.lua");
	}

	int ReleaseObj(lua_State* ls) {
		// ハンドルをデクリメント
		auto& sh = *reinterpret_cast<spn::SHandle*>(LCV<void*>()(-1,ls));
		spn::ResMgrBase::Release(sh);
		return 0;
	}
	void SetHandleMT(lua_State* ls) {
		LuaState lsc(ls);
		SetHandleMT(lsc);
	}
	void SetHandleMT(LuaState& lsc) {
		lsc.getGlobal(luaNS::ObjectBase);
		lsc.getField(-1, luaNS::objBase::UdataMT);
		lsc.setMetatable(-3);
		lsc.pop(1);
	}
	// ------------------- LCValue -------------------
	void LCV<SHandle>::operator()(lua_State* ls, SHandle h) const {
		// nullハンドルチェック
		if(!h.valid()) {
			// nilをpushする
			LCV<LuaNil>()(ls, LuaNil());
		} else {
			// ハンドルをインクリメント
			spn::ResMgrBase::Increment(h);
			// Luaのハンドルテーブルからオブジェクトの実体を取得
			LuaState lsc(ls);
			lsc.getGlobal(luaNS::GetHandle);
			lsc.push(h.getValue());
			lsc.call(1,1);
			AssertP(Trap, lsc.type(-1) == LuaType::Table)
		}
	}
	SHandle LCV<SHandle>::operator()(int idx, lua_State* ls) const {
		// userdata or nil
		LuaState lsc(ls);
		if(lsc.type(idx) == LuaType::Nil)
			return SHandle();
		return *reinterpret_cast<SHandle*>(LCV<void*>()(idx, ls));
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
	//	描画の登録
	//		ベースクラスが管理する(Luaからはベースクラスのメソッドを通して制御)
}

