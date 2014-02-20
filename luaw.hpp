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

class LuaState;
using SPLua = std::shared_ptr<LuaState>;
using WPLua = std::weak_ptr<LuaState>;
using SPILua = std::shared_ptr<lua_State>;
class LCValue;
using SPLCValue = std::shared_ptr<LCValue>;
class LCTable : public std::unordered_map<SPLCValue, SPLCValue> {
	public:
		using std::unordered_map<SPLCValue, SPLCValue>::unordered_map;
};
std::ostream& operator << (std::ostream& os, const LCTable& lct);
struct LuaNil {};
using SPLCTable = std::shared_ptr<LCTable>;
using LCVar = boost::variant<boost::blank, LuaNil, bool, const char*, lua_Integer, lua_Unsigned, lua_Number, SPLua, void*, lua_CFunction, const std::string&, std::string, LCTable>;

enum class LuaType {
	LNone,		//!< NoneだとX11のマクロと衝突する為
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

// (int, lua_State*)の順なのは、pushの時と引数が被ってしまう為
template <class T>
struct LCV;
#define DEF_LCV(rtyp, typ) template <> struct LCV<rtyp> { \
	void operator()(lua_State* ls, typ t) const; \
	rtyp operator()(int idx, lua_State* ls) const; \
	std::ostream& operator()(std::ostream& os, typ t) const; \
	LuaType operator()() const; };
DEF_LCV(boost::blank, boost::blank)
DEF_LCV(LuaNil, LuaNil)
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
DEF_LCV(LCValue, const LCValue&)
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
		LCValue(LCValue&& lcv): LCVar(reinterpret_cast<LCVar&&>(lcv)) {}
		LCValue(const std::string& str): LCVar(str) {}
		LCValue(int iv): LCVar(lua_Integer(iv)) {}
		LCValue(unsigned int iv): LCVar(lua_Unsigned(iv)) {}
		void push(lua_State* ls) const;
		std::ostream& print(std::ostream& os) const;
		LuaType type() const;
		friend std::ostream& operator << (std::ostream&, const LCValue&);
};
std::ostream& operator << (std::ostream& os, const LCValue& lcv);

//! lua_Stateの単純なラッパークラス
class LuaState : public std::enable_shared_from_this<LuaState> {
	template <class T>
	friend struct LCV;
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

		using Deleter = std::function<void (lua_State*)>;
		static Deleter _MakeDeleter(int id);

		static struct _TagThread {} TagThread;
		static SPILua _RegisterNewThread(LuaState& lsc, int id);
		static void _LoadThreadTable(LuaState& lsc);
		static void _Increment(LuaState& lsc, std::function<void (LuaState&)> cb);
		static void _Increment_ID(LuaState& lsc, int id);
		static void _Increment_Th(LuaState& lsc);
		static void _Decrement(LuaState& lsc, std::function<void (LuaState&)> cb);
		static void _Decrement_ID(LuaState& lsc, int id);
		static void _Decrement_Th(LuaState& lsc);
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
		void loadFromString(const std::string& code);
		void push(const LCValue& v);
		void pushCClosure(lua_CFunction func, int nvalue);
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

		// --- convert function ---
		bool toBoolean(int idx) const;
		lua_CFunction toCFunction(int idx) const;
		lua_Integer toInteger(int idx) const;
		std::string toString(int idx) const;
		lua_Number toNumber(int idx) const;
		const void* toPointer(int idx) const;
		lua_Unsigned toUnsigned(int idx) const;
		void* toUserData(int idx) const;
		SPLua toThread(int idx) const;
		LCTable toTable(int idx) const;
		LCValue toLCValue(int idx) const;

		template <class R>
		R toValue(int idx) const {
			return LCV<R>()(idx, getLS());
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
		SPLua getLS_SP();
		SPLua getMainLS_SP();
		static SPLua GetMainLS_SP(lua_State* ls);
		friend std::ostream& operator << (std::ostream& os, const LuaState&);
};
using SPLua = std::shared_ptr<LuaState>;
std::ostream& operator << (std::ostream& os, const LuaState& ls);

template <class T>
class LValue;
// LValue[], *LValueの時だけ生成される中間クラス
template <class LV, class IDX>
class LV_Inter {
	LValue<LV>&		_src;
	const IDX&		_index;
	public:
		LV_Inter(LValue<LV>& src, const IDX& index): _src(src), _index(index) {}
		LV_Inter(const LV_Inter&) = delete;
		LV_Inter(LV_Inter&& lv): _src(lv._src), _index(lv._index) {}

		void prepareValue(lua_State* ls) const {
			_src.prepareAt(ls, _index);
		}
		template <class VAL>
		LV_Inter& operator = (VAL&& v) {
			_src.setField(_index, std::forward<VAL>(v));
			return *this;
		}
		lua_State* getLS() {
			return _src.getLS();
		}
};
class LV_Global {
	const static std::string	cs_entry;
	static spn::FreeList<int>	s_index;
	SPLua		_lua;
	int			_id;

	void _init(const SPLua& sp);
	protected:
		// 代入する値をスタックの先頭に置いた状態で呼ぶ
		void _setValue();
		// 当該値をスタックに置く
		int _prepareValue() const;
		int _prepareValue(lua_State* ls) const;
		void _cleanValue() const;
		void _cleanValue(lua_State* ls) const;
	public:
		LV_Global(lua_State* ls);
		// スタックトップの値を管理対象とする
		LV_Global(const SPLua& sp);
		// 引数の値を管理対象とする
		LV_Global(const SPLua& sp, const LCValue& lcv);
		template <class LV>
		LV_Global(const SPLua& sp, const LValue<LV>& lv) {
			lv.prepareValue(sp->getLS());
			_init(sp);
		}
		LV_Global(const LV_Global& lv);
		LV_Global(LV_Global&& lv);
		~LV_Global();

		template <class LV>
		LV_Global& operator = (const LValue<LV>& lcv) {
			lua_State* ls = _lua->getLS();
			lcv.prepareValue(ls);
			return *this;
		}
		template <class T2>
		LV_Global& operator = (T2&& t) {
			_lua->getGlobal(cs_entry);
			_lua->push(_id);
			_lua->push(std::forward<T2>(t));
			_lua->setTable(-3);
			_lua->pop(1);
			return *this;
		}
		// lua_State*をゲットする関数
		lua_State* getLS() const;
};
class LV_Stack {
	lua_State*	_ls;
	int			_pos;

	protected:
		void _setValue();
		void _init(lua_State* ls);
		int _prepareValue() const;
		int _prepareValue(lua_State* ls) const;
		void _cleanValue() const;
		void _cleanValue(lua_State* ls) const;
	public:
		LV_Stack(lua_State* ls);
		LV_Stack(lua_State* ls, const LCValue& lcv);
		template <class LV, class IDX>
		LV_Stack(LV_Inter<LV,IDX>&& lv) {
			lua_State* ls = lv.getLS();
			lv.prepareValue(ls);
			_init(ls);
		}
		template <class LV>
		LV_Stack(lua_State* ls, const LValue<LV>& lv) {
			lv.prepareValue(ls);
			_init(ls);
		}
		LV_Stack(const LV_Stack& lv) = default;
		~LV_Stack();

		template <class LV>
		LV_Stack& operator = (const LValue<LV>& lcv) {
			lcv.prepareValue(_ls);
			lua_replace(_ls, _pos);
			return *this;
		}
		LV_Stack& operator = (const LCValue& lcv);
		LV_Stack& operator = (lua_State* ls);

		lua_State* getLS() const;
};

template <class T>
struct LValue_LCVT {
	using type = T;
};
template <int N>
struct LValue_LCVT<char [N]> {
	using type = const char*;
};
//! LuaState内部に値を保持する
template <class T>
class LValue : public T {
	struct VPop {
		const LValue& self;
		int			index;
		VPop(const LValue& s): self(s) {
			index = s._prepareValue();
		}
		operator int () const {
			return index;
		}
		~VPop() {
			self._cleanValue();
		}
	};
	public:
		using T::T;
		LValue(const LValue& lv): T(lv) {}
		LValue(LValue&& lv): T(std::move(lv)) {}
		LValue& operator = (const LCValue& lcv) {
			lcv.push(T::getLS());
			T::_setValue();
			return *this;
		}
		template <class LV>
		LValue& operator = (const LValue<LV>& lv) {
			return reinterpret_cast<LValue&>(T::operator =(lv));
		}
		LValue& operator = (LValue&& lv) {
			return reinterpret_cast<LValue&>(static_cast<T&>(*this) = std::move(lv));
		}
		template <class T2>
		LValue& operator = (T2&& t) {
			static_cast<T&>(*this) = std::forward<T2>(t);
			return *this;
		}
		template <class LV>
		LV_Inter<T, LValue<LV>> operator [](const LValue<LV>& lv) {
			return LV_Inter<T, LValue<LV>>(*this, lv);
		}
		LV_Inter<T, LCValue> operator [](const LCValue& lcv) {
			return LV_Inter<T, LCValue>(*this, lcv);
		}
		template <class... Ret, class... Args>
		void operator()(std::tuple<Ret...>& dst, Args&&... args) {
			LuaState lsc(T::getLS());
			T::prepareValue();
			// 引数をスタックに積む
			lsc.pushArgs(std::forward<Args>(args)...);
			lsc.call(sizeof...(Args), sizeof...(Ret));
			// 戻り値をtupleにセットする
			lsc.popValues(dst);
		}

		// --- convert function ---
		#define DEF_FUNC(typ, name) typ name() const { \
			return LCV<typ>()(VPop(*this), T::getLS()); }
		DEF_FUNC(bool, toBoolean)
		DEF_FUNC(lua_Integer, toInteger)
		DEF_FUNC(lua_Number, toNumber)
		DEF_FUNC(lua_Unsigned, toUnsigned)
		DEF_FUNC(void*, toUserData)
		DEF_FUNC(const char*, toString)
		DEF_FUNC(LCValue, toLCValue)
		DEF_FUNC(SPLua, toThread)
		#undef DEF_FUNC

		template <class IDX>
		void prepareAt(lua_State* ls, const IDX& idx) const {
			T::_prepareValue(ls);
			idx.push(ls);
			lua_gettable(ls, -2);
			lua_remove(ls, -2);
		}
		template <class IDX, class VAL>
		void setField(const IDX& idx, const VAL& val) {
			lua_State* ls = T::getLS();
			int n = T::_prepareValue();
			n = lua_absindex(ls, n);
			LCV<typename LValue_LCVT<IDX>::type>()(ls, idx);
			LCV<typename LValue_LCVT<VAL>::type>()(ls, val);
			lua_settable(ls, n);
			T::_cleanValue();
		}

		const void* toPointer() const {
			return lua_topointer(T::getLS(), VPop(*this));
		}
		template <class R>
		R toValue() const {
			return LCV<T>()(VPop(*this), T::getLS());
		}
		LuaType type() const {
			return LuaState::SType(T::getLS(), VPop(*this));
		}
};
using LValueS = LValue<LV_Stack>;
using LValueG = LValue<LV_Global>;

template <class... Ts0>
struct FuncCall {
	template <class T, class RT, class... Args, class... Ts1>
	static RT procMethod(lua_State* ls, T* ptr, int idx, RT (T::*func)(Args...), Ts1&&... ts1) {
		return (ptr->*func)(std::forward<Ts1>(ts1)...);
	}
	template <class RT, class... Args, class... Ts1>
	static RT proc(lua_State* ls, int idx, RT (*func)(Args...), Ts1&&... ts1) {
		return func(std::forward<Ts1>(ts1)...);
	}
};
template <class Ts0A, class... Ts0>
struct FuncCall<Ts0A, Ts0...> {
	template <class T, class RT, class... Args, class... Ts1>
	static RT procMethod(lua_State* ls, T* ptr, int idx, RT (T::*func)(Args...), Ts1&&... ts1) {
		return FuncCall<Ts0...>::procMethod(ls, ptr, idx+1, func, std::forward<Ts1>(ts1)..., LCV<Ts0A>()(idx, ls));
	}
	template <class RT, class... Args, class... Ts1>
	static RT proc(lua_State* ls, int idx, RT (*func)(Args...), Ts1&&... ts1) {
		return FuncCall<Ts0...>::proc(ls, idx+1, func, std::forward<Ts1>(ts1)..., LCV<Ts0A>()(idx, ls));
	}
};

//! Luaに返す値の数を型から特定する
template <class T>
struct RetSize {
	constexpr static int size = 1;
	static int proc(lua_State* ls, const T& t) {
		LCV<T>()(ls, t);
		return size;
	}
};
template <class... Ts>
struct RetSize<std::tuple<Ts...>> {
	constexpr static int size = sizeof...(Ts);
	static int proc(lua_State* ls, const std::tuple<Ts...>& t) {
		LCV<std::tuple<Ts...>>()(ls, t);
		return size;
	}
};
template <>
struct RetSize<void> {
	constexpr static int size = 0;
	static int proc(lua_State* ls) {
		return size;
	}
};
#define DEF_LUAIMPORT(Class, ...) \
	static const char* GetLuaName(); \
	static void ExportLua(LuaState& lsc);
#define DEF_REGMEMBER(n, clazz, elem)	LuaImport::RegisterMember(lsc, #elem, &clazz::elem);
#define DEF_LUAIMPLEMENT(clazz, seq_member, seq_method)	\
	const char* clazz::GetLuaName() { return #clazz; } \
	void clazz::ExportLua(LuaState& lsc) { \
		LuaImport::MakeLua(GetLuaName(), lsc); \
		lsc.newTable(); \
		BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_member) \
		lsc.pushValue(-1); \
		lsc.newTable(); \
		BOOST_PP_SEQ_FOR_EACH(DEF_REGMEMBER, clazz, seq_method) \
	}

// メンバ変数の時はtrue, それ以外はfalseを返す
template <class T>
struct IsMember {
	using type = std::false_type;
	constexpr static int value = 0;
};
template <class V, class T>
struct IsMember<V T::*> {
	using type = std::true_type;
	constexpr static int value = 1;
};

//! LuaへC++のクラスをインポート、管理する
class LuaImport {
	template <class PTR>
	static PTR _GetUD(lua_State* ls, int idx) {
		void** ud = reinterpret_cast<void**>(LCV<void*>()(idx, ls));
		return *reinterpret_cast<PTR*>(ud);
	}
	template <class V, class T>
	using PMember = V T::*;
	template <class RT, class T, class... Args>
	using PMethod = RT (T::*)(Args...);
	template <class V, class T>
	static PMember<V,T> GetMember(lua_State* ls, int idx) {
		return _GetUD<PMember<V,T>>(ls, idx);
	}
	template <class RT, class T, class... Args>
	static PMethod<RT,T,Args...> GetMethod(lua_State* ls, int idx) {
		return _GetUD<PMethod<RT,T,Args...>>(ls, idx);
	}
	// クラスの変数、関数ポインタは4byteでない可能性があるのでuserdataとして格納する
	template <class PTR>
	static void _SetUD(lua_State* ls, PTR ptr) {
		void* ud = lua_newuserdata(ls, sizeof(PTR));
		std::memcpy(ud, *reinterpret_cast<void**>(ptr), sizeof(PTR));
	}
	template <class T, class V>
	static void SetMember(lua_State* ls, V T::*member) {
		_SetUD(ls, &member);
	}
	template <class T, class RT, class... Args>
	static void SetMethod(lua_State* ls, RT (T::*method)(Args...)) {
		_SetUD(ls, &method);
	}
	public:
		//! 事前にClass_valueR, Class_valueW, Class_func, Class_Newを定義した状態で呼ぶ
		static void MakeLua(const char* name, LuaState& lsc);
		//! lscにFuncTableを積んだ状態で呼ぶ
		template <class RT, class T, class... Ts>
		static void RegisterMember(LuaState& lsc, const char* name, RT (T::*func)(Ts...)) {
			lsc.push(name);
			SetMethod(lsc.getLS(), func);
			lsc.pushCClosure(&CallMethod<RT,T,Ts...>, 1);
			lsc.setTable(-3);
		}
		template <class RT, class T, class... Ts>
		static void RegisterMember(LuaState& lsc, const char* name, RT (T::*func)(Ts...) const) {
			RegisterMember(lsc, name, (RT (T::*)(Ts...))func);
		}
		//! lscにReadTable, WriteTableを積んだ状態で呼ぶ
		template <class T, class V>
		static void RegisterMember(LuaState& lsc, const char* name, V T::*member) {
			// ReadValue関数の登録
			lsc.push(name);
			SetMember(lsc.getLS(), member);
			lsc.pushCClosure(&ReadValue<T,V>, 1);
			lsc.setTable(-4);
			// WriteValue関数の登録
			lsc.push(name);
			SetMember(lsc.getLS(), member);
			lsc.pushCClosure(&WriteValue<T,V>, 1);
			lsc.setTable(-3);
		}
		template <class T, class V>
		static int ReadValue(lua_State* ls) {
			// up[1]	変数ポインタ
			// [1]		クラスポインタ
			using VPtr = V T::*;
			const T* src = reinterpret_cast<const T*>(LCV<void*>()(1, ls));
			VPtr vp = GetMember<VPtr>(ls, lua_upvalueindex(1));
			LCV<V>()(ls, src->*vp);
			return 1;
		}
		template <class T, class V>
		static int WriteValue(lua_State* ls) {
			// up[1]	変数ポインタ
			// [1]		クラスポインタ
			// [2]		セットする値
			using VPtr = V T::*;
			const T* dst = reinterpret_cast<const T*>(LCV<void*>()(1, ls));
			VPtr ptr = GetMember<VPtr>(ls, lua_upvalueindex(1));
			(dst->*ptr) = LCV<V>()(2, ls);
			return 0;
		}
		template <class RT, class T, class... Args>
		static int CallMethod(lua_State* ls) {
			// up[1]	関数ポインタ
			// [1]		クラスポインタ
			// [2以降]	引数
			using F = RT (T::*)(Args...);
			void* tmp = lua_touserdata(ls, lua_upvalueindex(1));
			F f = *reinterpret_cast<F*>(&tmp);
			T* ptr = reinterpret_cast<T*>(LCV<void*>()(1, ls));
			return RetSize<RT>::proc(ls, FuncCall<Args...>::procMethod(ls, ptr, -sizeof...(Args), f));
		}
		template <class RT, class... Args>
		static int CallFunction(lua_State* ls) {
			// up[1]	関数ポインタ
			// [1]		クラスポインタ
			// [2以降]	引数
			using F = RT (*)(Args...);
			F f = reinterpret_cast<F>(lua_touserdata(ls, lua_upvalueindex(1)));
			// 引数を変換しつつ関数を呼んで、戻り値を変換しつつ個数を返す
			return RetSize<RT>::proc(ls, FuncCall<Args...>::proc(ls, -sizeof...(Args), f));
		}
		//! グローバル関数の登録
		template <class RT, class... Ts>
		static void RegisterFunction(LuaState& lsc, const char* name, RT (*func)(Ts...)) {
			lsc.push(reinterpret_cast<void*>(func));
			lsc.pushCClosure(&CallFunction<RT, Ts...>, 1);
			lsc.setGlobal(name);
		}
		//! クラスの登録(登録名はクラスから取得)
		template <class T>
		static void RegisterClass(LuaState& lsc) {
			lsc.newTable();
			lsc.setGlobal(T::GetLuaName());
			T::ExportLua(lsc);
		}
};

