#pragma once
#include "updator.hpp"
#include "luaw.hpp"

namespace rs {
	//! (LuaのClassT::New()から呼ばれる)オブジェクトのリソースハンドルを作成&メタテーブルの設定
	template <class T, class... Ts>
	int MakeObj(lua_State* ls) {
		auto fn = [](Ts&&... ts) {
			return mgr_gobj.makeObj<T>(std::forward<Ts>(ts)...);
		};
		HLGbj hlGbj = FuncCall<Ts...>::callCB(fn, ls, -sizeof...(Ts));
		using spn::SHandle;
		LuaState lsc(ls);
		// LightUserdataでハンドル値を保持
		SHandle sh = hlGbj.get();
		lsc.push(reinterpret_cast<void*>(sh.getValue()));
		// ハンドルが開放されてしまわないようにハンドル値だけリセットする
		hlGbj.setNull();
		// Userdataへのメタテーブル設定はC++からでしか行えないので、ここでする
		lsc.push(sh.getValue());
		return 2;
	}
}

