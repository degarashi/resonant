#pragma once

namespace rs {
	//! 単なる型のラッパー
	/*! Idか普通の数値かでオーバーロードしたい場合なんかに使用 */
	template <class T>
	struct Wrapper {
		T	value;
		Wrapper() = default;
		explicit Wrapper(const T& t): value(t) {}
		// 数値と明確に区別したいので暗黙的な変換はしない

		#define DEF_OP(op) \
			bool operator op (const T& t) const { return value op t; } \
			bool operator op (const Wrapper& w) const { return value op w.value; }
			DEF_OP(==)
			DEF_OP(!=)
			DEF_OP(<)
			DEF_OP(>)
			DEF_OP(<=)
			DEF_OP(>=)
		#undef DEF_OP
	};
}
