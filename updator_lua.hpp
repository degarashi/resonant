#pragma once
#include "updator.hpp"
#include "luaw.hpp"

namespace rs {
#define DEF_REGMEMBER(n, clazz, elem)	 rs::LuaImport::RegisterMember(lsc, BOOST_PP_STRINGIZE(elem), &clazz::elem);
#define DEF_LUAIMPLEMENT(clazz, seq_member, seq_method, seq_ctor)	\
		const char* clazz::getLuaName() const { return clazz::GetLuaName(); } \
		const char* clazz::GetLuaName() { return #clazz; } \
		void clazz::ExportLua(rs::LuaState& lsc) { \
			lsc.getGlobal(rs::luaNS::DerivedHandle); \
			lsc.getGlobal(rs::luaNS::ObjectBase); \
			lsc.call(1,1); \
			lsc.push(rs::luaNS::objBase::_New); \
			lsc.push(rs::MakeObj<BOOST_PP_SEQ_ENUM((clazz)seq_ctor)>); \
			lsc.setTable(-3); \
			\
			lsc.getField(-1, rs::luaNS::objBase::ValueR); \
			lsc.getField(-2, rs::luaNS::objBase::ValueW); \
			BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_member) \
			lsc.pop(2); \
			\
			lsc.getField(-1, rs::luaNS::objBase::Func); \
			BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_method) \
			lsc.pop(2); \
			lsc.setGlobal(#clazz); \
		}

	int ReleaseObj(lua_State* ls);
	template <class T, class... Ts>
	int MakeObj(lua_State* ls) {
		auto fn = [](Ts&&... ts) {
			return mgr_gobj.makeObj<T>(std::forward<Ts>(ts)...);
		};
		HLGbj hlGbj = FuncCall<Ts...>::callCB(fn, ls, -sizeof...(Ts));
		LCV<HLGbj>()(ls, hlGbj);

		// Userdataへのメタテーブル設定はC++からでしか行えないので、ここでする
		SetHandleMT(ls);
		return 1;
	}
}

