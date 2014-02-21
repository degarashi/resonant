#include "luaw.hpp"
#include "updator_lua.hpp"
#define BOOST_PP_VARIADICS 1
#include <boost/preprocessor.hpp>
#include <boost/regex.hpp>

namespace rs {
	using spn::SHandle;
	namespace {
		const std::string cs_base("\
			local _mt = { \
				__index = function(tbl, key) \
					local r = [[CLASS]]_valueR[key] \
					if r then return r(tbl) end \
					r = LuaHandleBase[key] \
					if r then return r(tbl) end \
					return [[CLASS]]_func[key] \
				end, \
				__newindex = function(tbl, key, value) \
					local r = [[CLASS]]_valueW[key] \
					if r then \
						r(tbl, value) \
					end \
				end, \
				__gc = [[CLASS]]_gc\
			} \
			[[CLASS]] = { \
				_mt = _mt, \
				New = function(...) \
					local h = [[CLASS]]_New(...) \
					return RegHandle(h.handleId, h) \
				end \
			}");
		//! リソースハンドルの登録・削除関数
		//! 事前にDeleteHandleを定義しておく
		const std::string cs_gbase("\
			local handleTable = {} \
			local handle_mt = { __gc = DeleteHandle } \
			setmetatable(handleTable, { \
				__mode = \"v\" \
			}) \
			function HasHandle(id) \
				return handleTable[id] \
			end \
			function RegHandle(id, ud) \
				handleTable[id] = ud \
				return ud \
			end \
			function DeleteHandle(ud) \
				for k,v in pairs(handleTable) do \
					if v == ud then \
						handleTable[k] = nil \
					end \
				end \
			end");
	}

	lua_Unsigned LuaImport::LuaHandleBase::HandleId(SHandle sh) {
		return sh.getValue(); }
	lua_Integer LuaImport::LuaHandleBase::NumRef(SHandle sh) {
		return sh.count(); }
	void LuaImport::LuaHandleBase::RegisterFuncs(LuaState& lsc) {
		lsc.newTable();

		lsc.push("handleId");
		LuaImport::PushFunction(lsc, &LuaHandleBase::HandleId);
		lsc.setTable(-3);
		lsc.push("numRef");
		LuaImport::PushFunction(lsc, &LuaHandleBase::NumRef);
		lsc.setTable(-3);

		lsc.setGlobal("LuaHandleBase");
	}

	int ReleaseObj(lua_State* ls) {
		// ハンドルをデクリメント
		auto& sh = *reinterpret_cast<spn::SHandle*>(LCV<void*>()(-1,ls));
		spn::ResMgrBase::Release(sh);
		return 0;
	}
	void SetHandleMT(lua_State* ls, const char* name) {
		LuaState lsc(ls);
		SetHandleMT(lsc, name); }
	void SetHandleMT(LuaState& lsc, const char* name) {
		lsc.getGlobal(name);
		lsc.getField(-1, "_mt");
		lsc.setMetatable(-3);
		lsc.pop(1);
	}
	void LuaImport::RegisterHandleFunc(LuaState& lsc) {
		lsc.loadFromString(cs_gbase);
		LuaHandleBase::RegisterFuncs(lsc);
	}
	void LuaImport::MakeLua(const char* name, LuaState& lsc) {
		boost::regex re("\\[\\[CLASS\\]\\]");
		// [[CLASS]] -> name
		std::string str = boost::regex_replace(cs_base, re, name);
		lsc.loadFromString(str);
	}
	using spn::SHandle;
	// ------------------- LCValue -------------------
	void LCV<SHandle>::operator()(lua_State* ls, SHandle h) const {
		// nullハンドルチェック
		if(!h.valid()) {
			// nilをpushする
			LCV<LuaNil>()(ls, LuaNil());
		} else {
			// Luaのハンドルテーブルで既存のハンドルがないかチェック
			LuaState lsc(ls);
			lsc.getGlobal("HasHandle");
			lsc.push(h.getValue());
			lsc.call(1,1);
			bool bHasHandle = lsc.type(-1) == LuaType::Userdata;
			if(bHasHandle) {
				// 既に登録してあったのでそのまま使う
			} else {
				// ハンドルをインクリメント
				spn::ResMgrBase::Increment(h);
				// エントリを作成
				lsc.pop(1);
				lsc.getGlobal("RegHandle");
				lsc.push(h.getValue());
				// LightUserdataは個別のメタテーブルを持てないのでUserdataを使う
				SHandle& sh = *reinterpret_cast<SHandle*>(lsc.newUserData(sizeof(SHandle)));
				sh = h;
				lsc.call(2,1);
			}
			std::cout << lsc;
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

