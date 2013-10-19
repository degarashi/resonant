#pragma once
#include <iostream>
#include <sstream>

#ifdef __ANDROID__
	#include <android/log.h>
	#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "threaded_app", __VA_ARGS__))
	#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "threaded_app", __VA_ARGS__))
	/* For debug builds, always enable the debug traces in this library */
	#ifndef NDEBUG
	#  define LOGV(...)  ((void)__android_log_print(ANDROID_LOG_VERBOSE, "threaded_app", __VA_ARGS__))
	#else
	#  define LOGV(...)  ((void)0)
	#endif
#endif

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
void CheckError(const char* filename, const char* funcname, int line, Ts&&... ts) {
	const char* msg = ALE::ErrorDesc(std::forward<Ts>(ts)...);
	if(msg) {
		std::cout << ALE::GetAPIName() << " Error:\t" <<  msg << std::endl
					<< "at file:\t" << filename << std::endl
					<< "at function:\t" << funcname << std::endl
					<< "on line:\t" << line << std::endl;
		if(A)
			__builtin_trap();
	}
}
template <bool A, class Chk>
struct ErrorChecker {
	int			_line;
	const char	*_filename, *_fname;
	ErrorChecker(const char* filename, const char* fname, int line): _filename(filename), _fname(fname), _line(line) {}
	~ErrorChecker() {
		CheckError<A, Chk>(_filename, _fname, _line);
	}
};
template <bool A, class Chk, class Func, class... TsA>
auto EChk_base(const char* filename, const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	ErrorChecker<A, Chk> chk(filename, fname, line);
	return func(std::forward<TsA>(ts)...);
}
template <bool A, class Chk>
void EChk_base(const char* filename, const char* fname, int line) {
	ErrorChecker<A, Chk> chk(filename, fname, line);
}
template <class Func, class... TsA>
auto EChk_pass(const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	return func(std::forward<TsA>(ts)...);
}
template <bool A, class Chk, class Func, class... TsA>
auto EChk_baseA1(const char* filename, const char* fname, int line, const Func& func, TsA&&... ts) -> decltype(func(std::forward<TsA>(ts)...)) {
	auto val = func(std::forward<TsA>(ts)...);
	CheckError<A,Chk>(filename, fname, line, val);
	return val;
}
template <bool A, class Chk, class RES>
RES EChk_baseA2(const char* filename, const char* fname, int line, const RES& res) {
	CheckError<A,Chk>(filename, fname, line, res);
	return res;
}
