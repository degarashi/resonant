#pragma once
#include "updator.hpp"
#include "luaw.hpp"

namespace rs {
#define DEF_REGMEMBER(n, clazz, elem)	 rs::LuaImport::RegisterMember(lsc, BOOST_PP_STRINGIZE(elem), &clazz::elem);
#define DEF_LUAIMPLEMENT(clazz, seq_member, seq_method, seq_ctor)	\
		const char* clazz::getLuaName() const { return clazz::GetLuaName(); } \
		const char* clazz::GetLuaName() { return #clazz; } \
		void clazz::ExportLua(rs::LuaState& lsc) { \
			lsc.push(rs::MakeObj<BOOST_PP_SEQ_ENUM((clazz)seq_ctor)>); \
			lsc.setGlobal(BOOST_PP_STRINGIZE(BOOST_PP_CAT(clazz, _New))); \
			lsc.push(rs::ReleaseObj); \
			lsc.setGlobal(BOOST_PP_STRINGIZE(BOOST_PP_CAT(clazz, _gc))); \
			lsc.newTable(); \
			lsc.newTable(); \
			BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_member) \
			lsc.setGlobal(BOOST_PP_STRINGIZE(BOOST_PP_CAT(clazz, _valueW))); \
			lsc.setGlobal(BOOST_PP_STRINGIZE(BOOST_PP_CAT(clazz, _valueR))); \
			lsc.newTable(); \
			BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_method) \
			lsc.setGlobal(BOOST_PP_STRINGIZE(BOOST_PP_CAT(clazz, _func))); \
			rs::LuaImport::MakeLua(clazz::GetLuaName(), lsc); \
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
		LuaState lsc(ls);
		SetHandleMT(lsc, T::GetLuaName());
		return 1;
	}
}

