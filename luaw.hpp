#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#define BOOST_PP_VARIADICS 1
#include <boost/variant.hpp>
#include "spinner/misc.hpp"
#include <lua.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <stack>
#include "spinner/type.hpp"

class LuaState;
using SPLua = std::shared_ptr<LuaState>;
using SPILua = std::shared_ptr<lua_State>;
class LCValue;
using SPLCValue = std::shared_ptr<LCValue>;
class LCTable : public std::unordered_map<SPLCValue, SPLCValue> {
	public:
		using std::unordered_map<SPLCValue, SPLCValue>::unordered_map;
		void pushValue(LuaState& ls) const;
		friend std::ostream& operator << (std::ostream&, const LCTable&);
};
std::ostream& operator << (std::ostream& os, const LCTable& lct);

using SPLCTable = std::shared_ptr<LCTable>;
using LCVar = boost::variant<boost::blank, bool, const char*, lua_Integer, lua_Unsigned, lua_Number, SPLua, void*, lua_CFunction, const std::string&, std::string, LCTable>;

enum class LuaType {
	None,
	Nil,
	Number,
	Boolean,
	String,
	Table,
	Function,
	Userdata,
	Thread,
	LightUserdata
};

template <class T>
struct LCV;
#define DEF_LCV(rtyp, typ) template <> struct LCV<rtyp> { \
	void operator()(lua_State* ls, typ t) const; \
	std::ostream& operator()(std::ostream& os, typ t) const; \
	LuaType operator()() const; };
DEF_LCV(boost::blank, boost::blank)
DEF_LCV(bool, bool)
DEF_LCV(const char*, const char*)
DEF_LCV(std::string, const std::string&)
DEF_LCV(lua_Integer, lua_Integer)
DEF_LCV(lua_Unsigned, lua_Unsigned)
DEF_LCV(lua_Number, lua_Number)
DEF_LCV(SPLua, const SPLua&)
DEF_LCV(void*, const void*)
DEF_LCV(lua_CFunction, lua_CFunction)
DEF_LCV(LCTable, const LCTable&)
#undef DEF_LCV

class LCValue : public LCVar {
	public:
		template <class T>
		LCValue(T& t): LCVar(t) {}
		template <class T>
		LCValue(T* t): LCVar(t) {}
		template <class T>
		LCValue(T&& t): LCVar(std::move(t)) {}
		LCValue(float fv): LCVar(lua_Number(fv)) {}
		LCValue(double dv): LCVar(lua_Number(dv)) {}
		void push(lua_State* ls) const;
		std::ostream& print(std::ostream& os) const;
		LuaType type() const;
		friend std::ostream& operator << (std::ostream&, const LCValue&);
};
std::ostream& operator << (std::ostream& os, const LCValue& lcv);

//! lua_Stateの単純なラッパークラス
class LuaState : public std::enable_shared_from_this<LuaState> {
	public:
		enum class CMP {
			Equal = LUA_OPEQ,
			LessThan = LUA_OPLT,
			LessEqual = LUA_OPLE
		};
		enum class OP {
			Add = LUA_OPADD,
			Sub = LUA_OPSUB,
			Mul = LUA_OPMUL,
			Div = LUA_OPDIV,
			Mod = LUA_OPMOD,
			Pow = LUA_OPPOW,
			Umm = LUA_OPUNM
		};
		enum class GC {
			Stop,
			Restart,
			Collect,
			Count,
			CountB,
			Step,
			SetPause,
			SetStepMul,
			IsRunning,
			Gen,
			Inc
		};

	private:
		const static std::string cs_fromId,
								cs_fromThread,
								cs_mainThread;
		const static int ENT_ID,
						ENT_THREAD,
						ENT_NREF;
		SPLua		_base;
		SPILua		_lua;
		int			_id;
		static spn::FreeList<int> s_index;
		static void Nothing(lua_State* ls);
		static void Delete(lua_State* ls);

		struct EBase : std::runtime_error {
			EBase(const std::string& typ_msg, const std::string& msg);
		};
		//! 実行時エラー
		struct ERun : EBase {
			ERun(const std::string& s);
		};
		//! コンパイルエラー
		struct ESyntax : EBase {
			ESyntax(const std::string& s);
		};
		//! メモリ割り当てエラー
		struct EMem : EBase {
			EMem(const std::string& s);
		};
		//! メッセージハンドラ実行中のエラー
		struct EError : EBase {
			EError(const std::string& s);
		};
		//! メタメソッド実行中のエラー
		struct EGC : EBase {
			EGC(const std::string& s);
		};
		//! 型エラー
		struct EType : EBase {
			EType(const char* typ0, const char* typ1);
		};

		//! Luaのエラーコードに応じて例外を投げる
		static void _CheckError(lua_State* ls, int code);
		void _checkError(int code) const;
		//! スタック位置idxの値がtypであるかチェック
		static void _CheckType(lua_State* ls, int idx, LuaType typ);
		void _checkType(int idx, LuaType typ) const;

		struct _TagThread {} TagThread;
		//! NewThread初期化
		LuaState(const SPLua& spLua);
		//! スレッド共有初期化
		LuaState(lua_State* ls, _TagThread);

	public:
		LuaState(lua_Alloc f=nullptr, void* ud=nullptr);
		LuaState(const LuaState&) = delete;
		LuaState(LuaState&& ls);
		LuaState(const SPILua& ls);
		LuaState(lua_State* ls);

		void load(const std::string& fname);
		void push(const LCValue& v);
		template <class A, class... Args>
		void pushArgs(A&& a, Args&&... args) {
			push(std::forward<A>(a));
			pushArgs(std::forward<Args>(args)...);
		}
		void pushArgs() {}
		template <class... Ret>
		std::tuple<Ret...> popValues() {
			std::tuple<Ret...> ret;
			return std::move(ret);
		}
		template <class... Ret>
		void popValues(std::tuple<Ret...>& dst) {
			using CT = spn::CType<Ret...>;
			popValues2<CT, 0>(dst, getTop());
		}
		template <class TUP, class CT, int N>
		void popValues2(TUP& dst, int nTop) {
			std::get<N>(dst) = toValue<typename CT::template At<N>::type>(nTop-N-1);
			popValues2<CT, N+1>(dst, nTop);
		}
		void pushValue(int idx);
		void pop(int n=1);
		void pushSelf();
		void pushGlobal();
		//! 標準ライブラリを読み込む
		void loadLibraries();

		int absIndex(int idx) const;
		void arith(OP op);
		lua_CFunction atPanic(lua_CFunction panicf);
		// 内部ではpcallに置き換え、エラーを検出したら例外を投げる
		void call(int nargs, int nresults);
		void callk(int nargs, int nresults, int ctx, lua_CFunction k);
		bool checkStack(int extra);
		bool compare(int idx1, int idx2, CMP cmp) const;
		void concat(int n);
		void copy(int from, int to);
		void dump(lua_Writer writer, void* data);
		void error();
		int gc(GC what, int data);
		lua_Alloc getAllocf(void** ud) const;
		int getCtx(int* ctx) const;
		void getField(int idx, const LCValue& key);
		void getGlobal(const LCValue& key);
		void getTable(int idx);
		int getTop() const;
		void getUserValue(int idx);
		void insert(int idx);
		bool isBoolean(int idx) const;
		bool isCFunction(int idx) const;
		bool isLightUserdata(int idx) const;
		bool isNil(int idx) const;
		bool isNone(int idx) const;
		bool isNoneOrNil(int idx) const;
		bool isNumber(int idx) const;
		bool isString(int idx) const;
		bool isTable(int idx) const;
		bool isThread(int idx) const;
		bool isUserdata(int idx) const;
		void length(int idx);
		void newTable(int narr=0, int nrec=0);
		LuaState newThread();
		void* newUserData(size_t sz);
		int next(int idx);
		bool rawEqual(int idx0, int idx1);
		void rawGet(int idx);
		void rawGet(int idx, const LCValue& v);
		size_t rawLen(int idx) const;
		void rawSet(int idx);
		void rawSet(int idx, const LCValue& v);
		void remove(int idx);
		void replace(int idx);
		bool resume(const SPLua& from, int narg=0);
		void setAllocf(lua_Alloc f, void* ud);
		void setField(int idx, const LCValue& key, const LCValue& val);
		void setGlobal(const LCValue& key);
		void setTable(int idx);
		void setTop(int idx);
		void setUservalue(int idx);
		bool status() const;

		// --- convert function (static) ---
		static bool ToBoolean(lua_State* ls, int idx);
		static lua_CFunction ToCFunction(lua_State* ls, int idx);
		static lua_Integer ToInteger(lua_State* ls, int idx);
		static std::pair<const char*, size_t> ToString(lua_State* ls, int idx);
		static lua_Number ToNumber(lua_State* ls, int idx);
		static const void* ToPointer(lua_State* ls, int idx);
		static lua_Unsigned ToUnsigned(lua_State* ls, int idx);
		static void* ToUserData(lua_State* ls, int idx);
		static SPLua ToThread(lua_State* ls, int idx);
		static LCValue ToLCValue(lua_State* ls, int idx);

		#define DEF_TOVALUE(func) static decltype(func(nullptr,0)) ToValue(lua_State* ls, int idx, decltype(func(nullptr,0))*) { \
			return func(ls, idx); }
		DEF_TOVALUE(ToBoolean)
		DEF_TOVALUE(ToInteger)
		DEF_TOVALUE(ToNumber)
		DEF_TOVALUE(ToUnsigned)
		DEF_TOVALUE(ToCFunction)
		DEF_TOVALUE(ToUserData)
		DEF_TOVALUE(ToLCValue)
		#undef DEF_TOVALUE

		template <class R>
		R ToValue(lua_State* ls, int idx) {
			return ToValue(ls, idx, (R*)nullptr);
		}

		// --- convert function ---
		bool toBoolean(int idx) const;
		lua_CFunction toCFunction(int idx) const;
		lua_Integer toInteger(int idx) const;
		std::pair<const char*, size_t> toString(int idx) const;
		lua_Number toNumber(int idx) const;
		const void* toPointer(int idx) const;
		lua_Unsigned toUnsigned(int idx) const;
		void* toUserData(int idx) const;
		SPLua toThread(int idx) const;
		LCValue toLCValue(int idx) const;

		template <class R>
		R toValue(int idx) const {
			return ToValue(getLS(), idx, (R*)nullptr);
		}

		LuaType type(int idx) const;
		static LuaType SType(lua_State* ls, int idx);
		const char* typeName(LuaType typ) const;
		static const char* STypeName(lua_State* ls, LuaType typ);
		const lua_Number* version() const;
		void xmove(const SPLua& to, int n);
		int yield(int nresults);
		int yieldk(int nresults, int ctx, lua_CFunction k);

		lua_State* getLS() const;
		SPLua getMainLS();
		friend std::ostream& operator << (std::ostream& os, const LuaState&);
};
using SPLua = std::shared_ptr<LuaState>;
std::ostream& operator << (std::ostream& os, const LuaState& ls);

