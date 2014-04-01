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
#include "spinner/resmgr.hpp"
#include "luaimport.hpp"

namespace rs {
	class RWops;
	class RWMgr;
	DEF_AHANDLE(rs::RWMgr, RW, rs::RWops, rs::RWops)

	struct LuaNil {
		bool operator == (LuaNil) const;
	};
}
namespace std {
	template <>
	struct hash<rs::LuaNil> {
		size_t operator()(rs::LuaNil) const {
			return 0; }
	};
	template <>
	struct hash<boost::blank> {
		size_t operator()(boost::blank) const {
			return 0; }
	};
}

namespace rs {
	template <class T>
	T& DeclVal() { return *(reinterpret_cast<T*>(0)); }
	class LuaState;
	using SPLua = std::shared_ptr<LuaState>;
	using WPLua = std::weak_ptr<LuaState>;
	using SPILua = std::shared_ptr<lua_State>;
	enum class LuaType {
		LNone = LUA_TNONE,	//!< NoneだとX11のマクロと衝突する為
		Nil = LUA_TNIL,
		Number = LUA_TNUMBER,
		Boolean = LUA_TBOOLEAN,
		String = LUA_TSTRING,
		Table = LUA_TTABLE,
		Function = LUA_TFUNCTION,
		Userdata = LUA_TUSERDATA,
		Thread = LUA_TTHREAD,
		LightUserdata = LUA_TLIGHTUSERDATA
	};

	class LCTable;
	class LCVec;
	using SPLCTable = std::shared_ptr<LCTable>;
	using LCVar = boost::variant<boost::blank, LuaNil,
					bool, const char*, float, double, int32_t, uint32_t, int64_t, uint64_t,
					spn::LHandle, spn::SHandle, spn::WHandle, SPLua, void*, lua_CFunction, const std::string&, std::string, SPLCTable>;
	class LCValue : public LCVar {
		public:
			struct HashVisitor : boost::static_visitor<size_t> {
				template <class T>
				size_t operator()(const T& t) const {
					return std::hash<T>()(t);
				}
			};
			LCValue();
			LCValue(const LCValue& lc);
			LCValue(LCValue&& lcv);
			template <class T>
			LCValue(T&& t): LCVar(std::forward<T>(t)) {}
			template <class T>
			LCValue& operator = (T&& t) {
				static_cast<LCVar&>(*this) = std::forward<T>(t);
				return *this;
			}
			LCValue& operator = (const LCValue& lcv);
			LCValue& operator = (LCValue&& lcv);
			bool operator == (const LCValue& lcv) const;
			void push(lua_State* ls) const;
			const char* toCStr() const;
			std::string toString() const;
			std::ostream& print(std::ostream& os) const;
			LuaType type() const;
			friend std::ostream& operator << (std::ostream&, const LCValue&);
	};
	std::ostream& operator << (std::ostream& os, const LCValue& lcv);
	using SPLCValue = std::shared_ptr<LCValue>;
}
namespace std {
	template <>
	struct hash<rs::LCValue> {
		size_t operator()(const rs::LCValue& v) const {
			return boost::apply_visitor(rs::LCValue::HashVisitor(), static_cast<const rs::LCVar&>(v));
		}
	};
}
namespace rs {
	class LCTable : public std::unordered_map<LCValue, LCValue> {
		public:
			using base = std::unordered_map<LCValue, LCValue>;
			using base::base;
	};
	std::ostream& operator << (std::ostream& os, const LCTable& lct);

	// (int, lua_State*)の順なのは、pushの時と引数が被ってしまう為
	template <class T>
	struct LCV;
	template <class T>
	class LValue;
	class LV_Global;
	using LValueG = LValue<LV_Global>;

	using LPointerSP = std::unordered_map<const void*, LCValue>;
#define DEF_LCV0(typ, rtyp, argtyp) template <> struct LCV<typ> { \
		void operator()(lua_State* ls, argtyp t) const; \
		rtyp operator()(int idx, lua_State* ls, LPointerSP* spm=nullptr) const; \
		std::ostream& operator()(std::ostream& os, argtyp t) const; \
		LuaType operator()() const; };
#define DEF_LCV(typ, argtyp) DEF_LCV0(typ, typ, argtyp)
#define DERIVED_LCV(ntyp, btyp)	template <> \
		struct LCV<ntyp> : LCV<btyp> {};
	DEF_LCV(boost::blank, boost::blank)
	DEF_LCV(LuaNil, LuaNil)
	DEF_LCV(bool, bool)
	DEF_LCV(const char*, const char*)
	DEF_LCV(std::string, const std::string&)
	DEF_LCV(SPLua, const SPLua&)
	DEF_LCV(void*, const void*)
	DEF_LCV(lua_CFunction, lua_CFunction)
	DEF_LCV0(LCTable, SPLCTable, const LCTable&)
	DEF_LCV(LCValue, const LCValue&)
	DEF_LCV(spn::SHandle, spn::SHandle)
	DEF_LCV(spn::WHandle, spn::WHandle)
	DEF_LCV(double, double)
	DEF_LCV(LValueG, const LValueG&)
	DERIVED_LCV(float, double)
	#if __x86_64__ || _LP64
		using Int_MainT = int64_t;
		using UInt_MainT = uint64_t;
		using Int_OtherT = int32_t;
		using UInt_OtherT = uint32_t;
	#else
		using Int_MainT = int32_t;
		using UInt_MainT = uint32_t;
		using Int_OtherT = int64_t;
		using UInt_OtherT = uint64_t;
	#endif
	DEF_LCV(Int_MainT, Int_MainT)
	DEF_LCV(UInt_MainT, UInt_MainT)
	DERIVED_LCV(Int_OtherT, Int_MainT)
	DERIVED_LCV(UInt_OtherT, UInt_MainT)
#undef DEF_LCV
#undef DEF_LCV0
#undef DERIVED_LCV
	template <class T>
	struct LCV<spn::HdlLock<T>> {
		using SH = decltype(std::declval<spn::HdlLock<T>>().get());
		void operator()(lua_State* ls, const spn::HdlLock<T>& t) const {
			LCV<SH>()(ls, t.get()); }
		spn::HdlLock<T> operator()(int idx, lua_State* ls) const {
			return LCV<SH>()(idx, ls); }
		std::ostream& operator()(std::ostream& os, const spn::HdlLock<T>& t) const {
			return LCV<SH>()(os, t.get()); }
		LuaType operator()() const {
			return LCV<SH>()(); }
	};

	template <class T>
	struct LCV<spn::SHandleT<T>> {
		void operator()(lua_State* ls, const spn::SHandleT<T>& t) const {
			LCV<spn::SHandle>()(ls, static_cast<spn::SHandle>(t));
		}
		spn::SHandleT<T> operator()(int idx, lua_State* ls) const {
			return  spn::SHandleT<T>::FromSHandle(LCV<spn::SHandle>()(idx, ls)); }
		std::ostream& operator()(std::ostream& os, const spn::SHandleT<T>& t) const {
			return LCV<spn::SHandle>()(os, static_cast<spn::SHandle>(t)); }
		LuaType operator()() const {
			return LCV<spn::SHandle>()(); }
	};

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

			//! RWopsに対するLua用のリーダークラス
			struct Reader {
				RWops&			ops;
				int64_t			size;
				spn::ByteBuff	buff;

				Reader(HRW hRW);
				static const char* Proc(lua_State* ls, void* data, size_t* size);
				static void Read(lua_State* ls, HRW hRW, const char* chunkName, const char* mode);
			};

		public:
			LuaState(lua_Alloc f=nullptr, void* ud=nullptr);
			LuaState(const LuaState&) = delete;
			LuaState(LuaState&& ls);
			LuaState(const SPILua& ls);
			LuaState(lua_State* ls);

			void load(HRW hRW, const char* chunkName=nullptr, const char* mode=cs_defaultmode, bool bExec=true);
			const static char* cs_defaultmode;
			//! ソースコードを読み取り、チャンクをスタックトップに積む
			/*! \param[in] bExec チャンクを実行するか */
			void loadFromSource(HRW hRW, const char* chunkName=nullptr, bool bExec=true);
			//! コンパイル済みバイナリを読み取り、チャンクをスタックトップに積む
			void loadFromBinary(HRW hRW, const char* chunkName=nullptr, bool bExec=true);
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
				popValues2<CT, sizeof...(Ret)-1>(dst, -1, typename spn::NType<0, sizeof...(Ret)>::less());
			}
			template <class CT, int N, class TUP>
			void popValues2(TUP& dst, int pos, std::false_type) {}
			template <class CT, int N, class TUP>
			void popValues2(TUP& dst, int pos, std::true_type) {
				std::get<N>(dst) = toValue<typename CT::template At<N>::type>(pos);
				popValues2<CT, N-1>(dst, pos-1, typename spn::NType<0, N-1>::less_eq());
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
			const char* getUpvalue(int idx, int n);
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
			void setMetatable(int idx);
			void setTable(int idx);
			void setTop(int idx);
			void setUservalue(int idx);
			const char* setUpvalue(int funcidx, int n);
			void* upvalueId(int funcidx, int n);
			void upvalueJoin(int funcidx0, int n0, int funcidx1, int n1);
			bool status() const;

			// --- convert function ---
			bool toBoolean(int idx) const;
			lua_CFunction toCFunction(int idx) const;
			Int_MainT toInteger(int idx) const;
			std::string toString(int idx) const;
			std::string cnvString(int idx);
			float toNumber(int idx) const;
			const void* toPointer(int idx) const;
			UInt_MainT toUnsigned(int idx) const;
			void* toUserData(int idx) const;
			SPLua toThread(int idx) const;
			LCTable toTable(int idx, LPointerSP* spm=nullptr) const;
			LCValue toLCValue(int idx, LPointerSP* spm=nullptr) const;

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
			friend std::ostream& operator << (std::ostream& os, const LV_Global& t);
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
			friend std::ostream& operator << (std::ostream& os, const LV_Stack& t);
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
			using Callback = std::function<void (LuaState&)>;
			void iterateTable(Callback cb) {
				LuaState lsc(T::getLS());
				int idx = T::_prepareValue();
				lsc.push(LuaNil());
				while(lsc.next(idx) != 0) {
					cb(lsc);
					lsc.pop(1);
				}
				T::_cleanValue();
			}
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
				T::_prepareValue();
				// 引数をスタックに積む
				lsc.pushArgs(std::forward<Args>(args)...);
				lsc.call(sizeof...(Args), sizeof...(Ret));
				// 戻り値をtupleにセットする
				lsc.popValues(dst);
			}
			template <class... Ret, class... Args>
			std::tuple<Ret...> call(Args&&... args) {
				std::tuple<Ret...> ret;
				this->operator()(ret, std::forward<Args>(args)...);
				return std::move(ret);
			}

			// --- convert function ---
			#define DEF_FUNC(typ, name) typ name() const { \
				return LCV<typ>()(VPop(*this), T::getLS()); }
			DEF_FUNC(bool, toBoolean)
			DEF_FUNC(Int_MainT, toInteger)
			DEF_FUNC(float, toNumber)
			DEF_FUNC(UInt_MainT, toUnsigned)
			DEF_FUNC(void*, toUserData)
			DEF_FUNC(const char*, toString)
			DEF_FUNC(LCValue, toLCValue)
			DEF_FUNC(SPLua, toThread)
			#undef DEF_FUNC

			void prepareValue(lua_State* ls) const {
				T::_prepareValue(ls);
			}
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
	std::ostream& operator << (std::ostream& os, const LV_Stack& t);
	std::ostream& operator << (std::ostream& os, const LV_Global& t);

	//! Luaから引数を変換取得して関数を呼ぶ
	template <class... Ts0>
	struct FuncCall {
		template <class CB, class... Ts1>
		static auto callCB(CB cb, lua_State* ls, int idx, Ts1&&... ts1) -> decltype(cb(std::forward<Ts1>(ts1)...)) {
			return cb(std::forward<Ts1>(ts1)...);
		}
		template <class T, class RT, class FT, class... Args, class... Ts1>
		static RT procMethod(lua_State* ls, T* ptr, int idx, RT (FT::*func)(Args...), Ts1&&... ts1) {
			return (ptr->*func)(std::forward<Ts1>(ts1)...);
		}
		template <class RT, class... Args, class... Ts1>
		static RT proc(lua_State* ls, int idx, RT (*func)(Args...), Ts1&&... ts1) {
			return func(std::forward<Ts1>(ts1)...);
		}
	};

	template <class T>
	using DecayT = typename std::decay<T>::type;
	template <class Ts0A, class... Ts0>
	struct FuncCall<Ts0A, Ts0...> {
		template <class CB, class... Ts1>
		static auto callCB(CB cb, lua_State* ls, int idx, Ts1&&... ts1) -> decltype(FuncCall<Ts0...>::callCB(cb, ls, idx+1, std::forward<Ts1>(ts1)..., LCV<DecayT<Ts0A>>()(idx, ls))) {
			DecayT<Ts0A> value = LCV<DecayT<Ts0A>>()(idx, ls);
			return FuncCall<Ts0...>::callCB(cb,
					ls,
					idx+1,
					std::forward<Ts1>(ts1)...,
					(Ts0A)value
					);
		}
		template <class T, class RT, class FT, class... Args, class... Ts1>
		static RT procMethod(lua_State* ls, T* ptr, int idx, RT (FT::*func)(Args...), Ts1&&... ts1) {
			DecayT<Ts0A> value = LCV<DecayT<Ts0A>>()(idx, ls);
			return FuncCall<Ts0...>::procMethod(ls,
					ptr,
					idx+1,
					func,
					std::forward<Ts1>(ts1)...,
					(Ts0A)value
					);
		}
		template <class RT, class... Args, class... Ts1>
		static RT proc(lua_State* ls, int idx, RT (*func)(Args...), Ts1&&... ts1) {
			DecayT<Ts0A> value = LCV<DecayT<Ts0A>>()(idx, ls);
			return FuncCall<Ts0...>::proc(ls,
					idx+1,
					func,
					std::forward<Ts1>(ts1)...,
					(Ts0A)value
					);
		}
	};

	//! Luaに返す値の数を型から特定する
	template <class T>
	struct RetSize {
		constexpr static int size = 1;
		template <class CB>
		static int proc(lua_State* ls, CB cb) {
			LCV<T>()(ls, cb());
			return size;
		}
	};
	template <class... Ts>
	struct RetSize<std::tuple<Ts...>> {
		constexpr static int size = sizeof...(Ts);
		template <class CB>
		static int proc(lua_State* ls, CB cb) {
			LCV<std::tuple<Ts...>>()(ls, cb());
			return size;
		}
	};
	template <>
	struct RetSize<void> {
		constexpr static int size = 0;
		template <class CB>
		static int proc(lua_State* ls, CB cb) {
			cb();
			return size;
		}
	};

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

	namespace luaNS {
		extern const std::string Udata,
								Pointer,
								ToString;
		extern const std::string GetHandle,
								DeleteHandle,
								ObjectBase,
								DerivedHandle,
								MakeFSMachine,
								MakePreENV,
								RecvMsg,
								System;
		namespace objBase {
			extern const std::string ValueR,
									ValueW,
									Func,
									UdataMT,
									MT,
									_New;
			namespace valueR {
				extern const std::string HandleId,
										NumRef;
			}
		}
	}
}
DEF_LUAIMPORT_BASE
namespace rs {
	// --- Lua->C++グルーコードにおけるクラスポインタの取得方法 ---
	//! "pointer"に生ポインタが記録されている
	struct LI_GetPtr {
		void* operator()(lua_State* ls, int idx) const;
	};
	struct LI_GetHandleBase {
		void* operator()(lua_State* ls, int idx) const;
		spn::SHandle getHandle(lua_State* ls, int idx) const;
	};
	//! "udata"にハンドルが記録されている -> void*からそのままポインタ変換
	template <class T>
	struct LI_GetHandle : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			return reinterpret_cast<T*>(static_cast<const LI_GetHandleBase&>(*this)(ls, idx));
		}
	};
	//! "udata"にハンドルが記録されている -> unique_ptrからポインタ変換
	template <class T, class Deleter>
	struct LI_GetHandle<std::unique_ptr<T, Deleter>> : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			auto* up = reinterpret_cast<std::unique_ptr<T,Deleter>*>(
				static_cast<const LI_GetHandleBase&>(*this)(ls, idx));
			return up->get();
		}
	};
	//! "udata"にハンドルが記録されている -> shared_ptrからポインタ変換
	template <class T>
	struct LI_GetHandle<std::shared_ptr<T>> : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			auto* up = reinterpret_cast<std::shared_ptr<T>*>(
				static_cast<const LI_GetHandleBase&>(*this)(ls, idx));
			return up->get();
		}
	};
	//! LuaへC++のクラスをインポート、管理する
	class LuaImport {
		//! ハンドルオブジェクトの基本メソッド
		static lua_Unsigned HandleId(spn::SHandle sh);
		static lua_Integer NumRef(spn::SHandle sh);

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
			std::memcpy(ud, reinterpret_cast<void**>(&ptr), sizeof(PTR));
		}
		template <class T, class V>
		static void SetMember(lua_State* ls, V T::*member) {
			_SetUD(ls, member);
		}
		template <class T, class RT, class... Args>
		static void SetMethod(lua_State* ls, RT (T::*method)(Args...)) {
			_SetUD(ls, method);
		}
		public:
			//! lscにFuncTableを積んだ状態で呼ぶ
			template <class GET, class T, class RT, class FT, class... Ts>
			static void RegisterMember(LuaState& lsc, const char* name, RT (FT::*func)(Ts...)) {
				lsc.push(name);
				SetMethod(lsc.getLS(), func);
				lsc.pushCClosure(&CallMethod<GET,T,RT,FT,Ts...>, 1);
				lsc.setTable(-3);
			}
			template <class GET, class T, class RT, class FT, class... Ts>
			static void RegisterMember(LuaState& lsc, const char* name, RT (FT::*func)(Ts...) const) {
				RegisterMember<GET,T>(lsc, name, (RT (T::*)(Ts...))func);
			}
			//! lscにReadTable, WriteTableを積んだ状態で呼ぶ
			template <class GET, class T, class V, class VT>
			static void RegisterMember(LuaState& lsc, const char* name, V VT::*member) {
				// ReadValue関数の登録
				lsc.push(name);
				SetMember(lsc.getLS(), member);
				lsc.pushCClosure(&ReadValue<GET,T,V,VT>, 1);
				lsc.setTable(-4);
				// WriteValue関数の登録
				lsc.push(name);
				SetMember(lsc.getLS(), member);
				lsc.pushCClosure(&WriteValue<GET,T,V,VT>, 1);
				lsc.setTable(-3);
			}
			//! luaスタックから変数ポインタとクラスを取り出しメンバ変数を読み込む
			template <class GET, class T, class V, class VT>
			static int ReadValue(lua_State* ls) {
				// up[1]	変数ポインタ
				// [1]		クラスポインタ(userdata)
				const T* src = reinterpret_cast<const T*>(GET()(ls, 1));
				auto vp = GetMember<V,VT>(ls, lua_upvalueindex(1));
				LCV<V>()(ls, src->*vp);
				return 1;
			}
			//! luaスタックから変数ポインタとクラスと値を取り出しメンバ変数に書き込む
			template <class GET, class T, class V, class VT>
			static int WriteValue(lua_State* ls) {
				// up[1]	変数ポインタ
				// [1]		クラスポインタ(userdata)
				// [2]		セットする値
				T* dst = reinterpret_cast<T*>(GET()(ls, 1));
				auto ptr = GetMember<V,VT>(ls, lua_upvalueindex(1));
				(dst->*ptr) = LCV<V>()(2, ls);
				return 0;
			}
			//! luaスタックから関数ポインタとクラス、引数を取り出しクラスのメンバ関数を呼ぶ
			template <class GET, class T, class RT, class FT, class... Args>
			static int CallMethod(lua_State* ls) {
				// up[1]	関数ポインタ
				// [1]		クラスポインタ(userdata)
				// [2以降]	引数
				using F = RT (FT::*)(Args...);
				void* tmp = lua_touserdata(ls, lua_upvalueindex(1));
				F f = *reinterpret_cast<F*>(tmp);
				auto* ptr = static_cast<T*>(GET()(ls, 1));
				return RetSize<RT>::proc(ls, [ls,ptr,f]() { return FuncCall<Args...>::procMethod(ls, ptr, 2, f); });
			}
			//! luaスタックから関数ポインタと引数を取り出しcall
			template <class RT, class... Args>
			static int CallFunction(lua_State* ls) {
				// up[1]	関数ポインタ
				// [1以降]	引数
				using F = RT (*)(Args...);
				F f = reinterpret_cast<F>(lua_touserdata(ls, lua_upvalueindex(1)));
				// 引数を変換しつつ関数を呼んで、戻り値を変換しつつ個数を返す
				return RetSize<RT>::proc(ls, [ls,f](){ return FuncCall<Args...>::proc(ls, 1, f); });
			}
			//! staticな関数をスタックへpush
			template <class RT, class... Ts>
			static void PushFunction(LuaState& lsc, RT (*func)(Ts...)) {
				lsc.push(reinterpret_cast<void*>(func));
				lsc.pushCClosure(&CallFunction<RT, Ts...>, 1);
			}
			//! グローバル関数の登録
			template <class RT, class... Ts>
			static void RegisterFunction(LuaState& lsc, const char* name, RT (*func)(Ts...)) {
				PushFunction(lsc, func);
				lsc.setGlobal(name);
			}
			//! C++クラスの登録(登録名はクラスから取得)
			template <class T>
			static void RegisterBaseClass(LuaState& lsc) {
				lua::LuaExport(lsc, static_cast<T*>(nullptr));
			}
			//! C++クラス登録基盤を初期化
			static void RegisterObjectBase(LuaState& lsc);
			//! ベースオブジェクトを使った派生クラスの読み込み
			/*! 1クラス1ファイルの対応
				ベースクラスの名前はファイルに記載 */
			static void LoadClass(LuaState& lsc, const std::string& name, HRW hRW);
			//! 固有オブジェクトのインポート
			/*! ポインタ指定でLuaにクラスを取り込む
				リソースマネージャやシステムクラス用 */
			template <class T>
			static void ImportClass(LuaState& lsc, const std::string& name, T* ptr) {
				auto* dummy = static_cast<T*>(nullptr);
				lua::LuaExport(lsc, dummy);
				lsc.getGlobal(lua::LuaName(dummy));
				lsc.getField(-1, "ConstructPtr");
				lsc.push(static_cast<void*>(ptr));
				lsc.call(1,1);
				lsc.setGlobal(name);
				lsc.pop(1);
			}
			//! GObjectメッセージを受信
			/*! Obj(UData), MessageStr, {Args} */
			static int RecvMsg(lua_State* ls);
	};
	struct LSysFunc {
		static void InitFuncs(LuaState& lsc);

		//! 拡張子で型を判別してリソース読み込み
		static spn::SHandle LoadResourceFromURI(const std::string& urisrc);
		//! Luaのテーブルからリソースを一括読み込み
		static LCTable LoadResources(LValueG tbl);
	};
}

