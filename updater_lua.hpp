#pragma once
#include "updater.hpp"
#include "luaw.hpp"

namespace rs {
	namespace detail {
		template <class Mgr_t, class TF>
		struct MakeHandle_Tmp {
			template <class... Ts>
			auto operator()(Ts&&... ts) {
				return Mgr_t::_ref().template makeHandle<TF>(std::forward<Ts>(ts)...);
			}
		};
		template <class T>
		struct MakeObject_Tmp {
			template <class... Ts>
			auto operator()(Ts&&... ts) {
				return T(std::forward<Ts>(ts)...);
			}
		};
	}
	//! (LuaのClassT::New()から呼ばれる)オブジェクトのリソースハンドルを作成
	template <class Mgr_t, class TF, class... Ts>
	int MakeHandle(lua_State* ls) {
		detail::MakeHandle_Tmp<Mgr_t, TF> fn;
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
	template <class T, class... Ts>
	int MakeObject(lua_State* ls) {
		detail::MakeObject_Tmp<T> fn;
		T ret = FuncCall<Ts...>::callCB(fn, ls, static_cast<int>(-sizeof...(Ts)));
		LCV<T>()(ls, ret);
		return 1;
	}
}
