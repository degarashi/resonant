#pragma once
#include <iostream>

#define Assert(expr) AssertMsg(expr, "Assertion failed")
#define AssertMsg(expr, msg) { if(!(expr)) throw std::runtime_error(msg); }
#ifdef DEBUG
	#define AAssert(expr)			AAssertMsg(expr, "Assertion failed")
	#define AAssertMsg(expr, msg)	AssertMsg(expr, msg)
#else
	#define AAssert(expr)
	#define AAssertMsg(expr,msg)
#endif

template <bool A, class ALE, class... Ts>
void CheckError(const char* funcname, int line, Ts&&... ts) {
	const char* msg = ALE::ErrorDesc(std::forward<Ts>(ts)...);
	if(msg) {
		std::cout << ALE::GetAPIName() << " Error:\t" <<  msg << std::endl
					<< "at function:\t" << funcname << std::endl
					<< "on line:\t" << line << std::endl;
		if(A)
			__asm__("int $0x03;");
	}
}
template <bool A, class Chk>
struct ErrorChecker {
	int			_line;
	const char*	_fname;
	ErrorChecker(const char* fname, int line): _fname(fname), _line(line) {}
	~ErrorChecker() {
		CheckError<A, Chk>(_fname, _line);
	}
};
template <bool A, class Chk, class Func, class... TsA>
auto EChk_base(const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	ErrorChecker<A, Chk> chk(fname, line);
	return func(std::forward<TsA>(ts)...);
}
template <bool A, class Chk>
void EChk_base(const char* fname, int line) {
	ErrorChecker<A, Chk> chk(fname, line);
}
template <class Func, class... TsA>
auto EChk_pass(const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	return func(std::forward<TsA>(ts)...);
}
template <bool A, class Chk, class Func, class... TsA>
auto EChk_baseA1(const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	auto val = func(std::forward<TsA>(ts)...);
	CheckError<A,Chk>(fname, line, val);
	return val;
}
