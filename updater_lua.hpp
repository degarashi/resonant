#pragma once
#include "updater.hpp"
#include "luaw.hpp"

namespace rs {
	//! (LuaのClassT::New()から呼ばれる)オブジェクトのリソースハンドルを作成
	template <class Mgr_t, class... Ts>
	int MakeHandle(lua_State* ls) {
		auto fn = [](auto&&... ts) {
			return Mgr_t::_ref().makeHandle(std::forward<typename std::decay<decltype(ts)>::type>(ts)...);
		};
		auto hlObj = FuncCall<Ts...>::callCB(fn, ls, static_cast<int>(-sizeof...(Ts)));
		LuaState lsc(ls);
		// LightUserdataでハンドル値を保持
		spn::SHandle sh = hlObj.get();
		lsc.push(reinterpret_cast<void*>(sh.getValue()));
		// ハンドルが開放されてしまわないようにハンドル値だけリセットする
		hlObj.setNull();
		return 1;
	}
	template <class... Ts>
	int MakeHandle_Fake(lua_State* /*ls*/) {
		Assert(Trap, false, "not constructor defined.(can't construct this type of object)")
		return 0;
	}
}
