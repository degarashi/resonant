#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include <boost/variant.hpp>
#include "spinner/misc.hpp"
#include <lua.hpp>
#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include "spinner/resmgr.hpp"
#include "spinner/structure/freeobj.hpp"
#include "luaimport.hpp"
#include "handle.hpp"
#include "glformat.hpp"
#include "clock.hpp"
#include "spinner/structure/angle.hpp"

DEF_LUAIMPORT(spn::Vec2)
DEF_LUAIMPORT(spn::Vec3)
DEF_LUAIMPORT(spn::Vec4)
DEF_LUAIMPORT(spn::Mat22)
DEF_LUAIMPORT(spn::Mat33)
DEF_LUAIMPORT(spn::Mat44)
DEF_LUAIMPORT(spn::Quat)

DEF_LUAIMPORT(spn::AVec2)
DEF_LUAIMPORT(spn::AVec3)
DEF_LUAIMPORT(spn::AVec4)
DEF_LUAIMPORT(spn::AMat22)
DEF_LUAIMPORT(spn::AMat33)
DEF_LUAIMPORT(spn::AMat44)
DEF_LUAIMPORT(spn::AQuat)

DEF_LUAIMPORT(spn::DegF)
DEF_LUAIMPORT(spn::RadF)

namespace rs {
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
namespace spn {
	template <class T>
	struct _Size;
	using Size = _Size<int32_t>;
	using SizeF = _Size<float>;
}

namespace rs {
	double LuaOtherNumber(std::integral_constant<int,4>);
	float LuaOtherNumber(std::integral_constant<int,8>);
	int32_t LuaOtherInteger(std::integral_constant<int,8>);
	int64_t LuaOtherInteger(std::integral_constant<int,4>);

	//! lua_Numberがfloatならdouble、doubleならfloat
	using lua_OtherNumber = decltype(LuaOtherNumber(std::integral_constant<int,sizeof(lua_Number)>()));
	//! lua_Integerがint32_tならint64_t、int64_tならint32_t
	using lua_OtherInteger = decltype(LuaOtherInteger(std::integral_constant<int,sizeof(lua_Integer)>()));
	using lua_IntegerU = std::make_unsigned<lua_Integer>::type;
	using lua_OtherIntegerU = std::make_unsigned<lua_OtherInteger>::type;

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
	using SPLCTable = std::shared_ptr<LCTable>;
	using LCVar = boost::variant<boost::blank, LuaNil,
					bool, const char*, lua_Integer, lua_Number,
					spn::Vec2, spn::Vec3, spn::Vec4, spn::Quat,
					spn::LHandle, spn::SHandle, spn::WHandle, SPLua, void*, lua_CFunction, std::string, SPLCTable>;
	class LCValue : public LCVar {
		private:
			spn::SHandle _toHandle(spn::SHandle*) const;
			spn::WHandle _toHandle(spn::WHandle*) const;
			template <int N>
			using IConst = std::integral_constant<int, N>;
			template <class... Args, int N, typename std::enable_if<N==sizeof...(Args)>::type*& = spn::Enabler>
			static void _TupleAsTable(SPLCTable&, const std::tuple<Args...>&, IConst<N>) {}
			template <class... Args, int N, typename std::enable_if<N!=sizeof...(Args)>::type*& = spn::Enabler>
			static void _TupleAsTable(SPLCTable& tbl, const std::tuple<Args...>& t, IConst<N>);
			template <class... Args>
			static SPLCTable _TupleAsTable(const std::tuple<Args...>& t);
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
			LCValue(lua_OtherNumber num);
			LCValue(lua_IntegerU num);
			LCValue(lua_OtherIntegerU num);
			LCValue(lua_OtherInteger num);

			// Tupleは配列に変換
			LCValue(std::tuple<>& t);
			LCValue(std::tuple<>&& t);
			LCValue(const std::tuple<>& t);
			template <class... Args>
			LCValue(std::tuple<Args...>& t);
			template <class... Args>
			LCValue(std::tuple<Args...>&& t);
			template <class... Args>
			LCValue(const std::tuple<Args...>& t);
			template <int N, bool B>
			LCValue(const spn::VecT<N,B>& v): LCVar(static_cast<const spn::VecT<N,false>&>(v)) { }
			//! 任意のベクトルからVec4型への代入
			template <int N, bool B>
			LCValue& operator = (const spn::VecT<N,B>& v) {
				return *this = LCValue(static_cast<const spn::VecT<N,false>&>(v));
			}
			LCValue& operator = (const LCValue& lcv);
			LCValue& operator = (LCValue&& lcv);
			template <class T>
			constexpr static auto IsHandleT = spn::IsHandleT<std::decay_t<T>>::value;
			template <class T>
			constexpr static auto IsVectorT = spn::IsVectorT<std::decay_t<T>>::value;
			// リソースの固有ハンドルは汎用へ読み替え
			// >>SHandle & WHandle
			template <class H, class=std::enable_if_t<IsHandleT<H>>>
			LCValue(const H& h): LCVar(h.getBase()) {}
			template <class H, class=std::enable_if_t<IsHandleT<H>>>
			LCValue& operator = (const H& h) {
				*this = h.getBase();
				return *this;
			}
			// >> LHandle
			template <class H, bool B, class=std::enable_if_t<IsHandleT<H>>>
			LCValue(const spn::HdlLock<H,B>& h): LCVar(spn::LHandle(h)) {}
			template <class H, bool B, class=std::enable_if_t<IsHandleT<H>>>
			LCValue& operator = (const spn::HdlLock<H,B>& h) {
				*this = spn::LHandle(h);
				return *this;
			}
			// それ以外はそのままLCVarに渡す
			template <class T, class=std::enable_if_t<!IsHandleT<T> && !IsVectorT<T>>>
			LCValue(T&& t): LCVar(std::forward<T>(t)) {}
			template <class T, class=std::enable_if_t<!IsHandleT<T> && !IsVectorT<T>>>
			LCValue& operator = (T&& t) {
				static_cast<LCVar&>(*this) = std::forward<T>(t);
				return *this;
			}
			//! 中身を配列とみなしてアクセス
			/*! \param[in] n 0オリジンのインデックス */
			const LCValue& operator[] (int n) const;

			//! SHandleやLHandleを任意のハンドルへ変換して取り出す
			template <class H>
			auto toHandle() const {
				using handle_t = decltype(std::declval<H>().getBase());
				auto h = _toHandle(static_cast<handle_t*>(nullptr));
				return H::FromHandle(h);
			}
			bool operator == (const LCValue& lcv) const;
			bool operator != (const LCValue& lcv) const;
			//! blank, Nilかfalseの場合にのみfalse, それ以外はtrueを返す
			explicit operator bool () const;
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
namespace spn {
	class Pose2D;
	class Pose3D;
}
namespace rs {
	class LCTable : public std::unordered_map<LCValue, LCValue> {
		public:
			using base = std::unordered_map<LCValue, LCValue>;
			using base::base;
	};
	std::ostream& operator << (std::ostream& os, const LCTable& lct);

	template <class... Args, int N, typename std::enable_if<N!=sizeof...(Args)>::type*&>
	void LCValue::_TupleAsTable(SPLCTable& tbl, const std::tuple<Args...>& t, IConst<N>) {
		tbl->emplace(N+1, std::get<N>(t));
		_TupleAsTable(tbl, t, IConst<N+1>());
	}
	template <class... Args>
	SPLCTable LCValue::_TupleAsTable(const std::tuple<Args...>& t) {
		auto ret = std::make_shared<LCTable>();
		LCValue::_TupleAsTable(ret, t, IConst<0>());
		return ret;
	}
	template <class... Args>
	LCValue::LCValue(std::tuple<Args...>& t): LCValue(static_cast<const std::tuple<Args...>&>(t)) {}
	template <class... Args>
	LCValue::LCValue(std::tuple<Args...>&& t): LCValue(static_cast<const std::tuple<Args...>&>(t)) {}
	template <class... Args>
	LCValue::LCValue(const std::tuple<Args...>& t): LCVar(_TupleAsTable(t)) {}

	using LPointerSP = std::unordered_map<const void*, LCValue>;
#define DEF_LCV_OSTREAM2(typ, name)	std::ostream& operator << (std::ostream& os, LCV<typ>) { return os << #name; }
#define DEF_LCV_OSTREAM(typ)		DEF_LCV_OSTREAM2(typ, typ)
	// 値型の場合はUserdataにデータを格納
	template <class T>
	struct LCVRaw;
	template <class T>
	struct LCVRaw<T*>;
	template <class T>
	struct LCVRaw<T&>;
	template <class T>
	struct LCV;
	template <>
	struct LCV<void> {};
	std::ostream& operator << (std::ostream& os, LCV<void>);
	std::ostream& operator << (std::ostream& os, LCV<lua_OtherNumber>);
	std::ostream& operator << (std::ostream& os, LCV<lua_OtherInteger>);
	std::ostream& operator << (std::ostream& os, LCV<spn::DegF>);
	std::ostream& operator << (std::ostream& os, LCV<spn::RadF>);
	std::ostream& operator << (std::ostream& os, LCV<spn::Pose2D>);
	std::ostream& operator << (std::ostream& os, LCV<spn::Pose3D>);
	template <bool A>
	std::ostream& operator << (std::ostream& os, LCV<spn::PlaneT<A>>) {
		if(A)
			os << "A";
		return os << "Plane";
	}
	template <bool A>
	std::ostream& operator << (std::ostream& os, LCV<spn::QuatT<A>>) {
		if(A)
			os << "A";
		return os << "Quat";
	}
	template <int N, bool A>
	std::ostream& operator << (std::ostream& os, LCV<spn::VecT<N,A>>) {
		if(A)
			os << "A";
		return os << "Vec" << N;
	}
	template <int M, int N, bool A>
	std::ostream& operator << (std::ostream& os, LCV<spn::MatT<M,N,A>>) {
		if(A)
			os << "A";
		return os << "Mat" << M << N;
	}
	template <class T>
	class LValue;
	class LV_Global;
	using LValueG = LValue<LV_Global>;
	class LV_Stack;
	using LValueS = LValue<LV_Stack>;

#define DEF_LCV0(typ, rtyp, argtyp) template <> struct LCV<typ> { \
		int operator()(lua_State* ls, argtyp t) const; \
		rtyp operator()(int idx, lua_State* ls, LPointerSP* spm=nullptr) const; \
		std::ostream& operator()(std::ostream& os, argtyp t) const; \
		LuaType operator()() const; }; \
		std::ostream& operator << (std::ostream& os, LCV<typ>);
#define DEF_LCV(typ, argtyp) DEF_LCV0(typ, typ, argtyp)
#define DERIVED_LCV(ntyp, btyp)	template <> \
		struct LCV<ntyp> : LCV<btyp> {};
	DEF_LCV(boost::blank, boost::blank)
	DEF_LCV(LuaNil, LuaNil)
	DEF_LCV(bool, bool)
	DEF_LCV(const char*, const char*)
	DEF_LCV(std::string, const std::string&)
	DEF_LCV(spn::DegF, const spn::DegF&)
	DEF_LCV(spn::RadF, const spn::RadF&)
	DERIVED_LCV(const std::string&, std::string)
	DEF_LCV(lua_State*, lua_State*)
	DEF_LCV(spn::SizeF, const spn::SizeF&)
	DERIVED_LCV(spn::Size, spn::SizeF)
	DEF_LCV(spn::RectF, const spn::RectF&)
	DERIVED_LCV(spn::Rect, spn::RectF)
	DEF_LCV(SPLua, const SPLua&)
	DEF_LCV(void*, const void*)
	DEF_LCV(lua_CFunction, lua_CFunction)
	DEF_LCV(Timepoint, const Timepoint&)
	DEF_LCV0(LCTable, SPLCTable, const LCTable&)
	DEF_LCV(LCValue, const LCValue&)
	DEF_LCV(spn::SHandle, spn::SHandle)
	DEF_LCV(spn::WHandle, spn::WHandle)
	DEF_LCV(LValueG, const LValueG&)
	DEF_LCV(LValueS, const LValueS&)
	DEF_LCV(lua_Number, lua_Number)
	DERIVED_LCV(lua_OtherNumber, lua_Number)
	DEF_LCV(lua_Integer, lua_Integer)
	DERIVED_LCV(lua_IntegerU, lua_Integer)
	DERIVED_LCV(lua_OtherInteger, lua_Integer)
	DERIVED_LCV(lua_OtherIntegerU, lua_OtherInteger)
	DERIVED_LCV(long, lua_Integer)

	DERIVED_LCV(GLFormat, lua_Integer)
	DERIVED_LCV(GLDepthFmt, GLFormat)
	DERIVED_LCV(GLStencilFmt, GLFormat)
	DERIVED_LCV(GLDSFmt, GLFormat)
	DERIVED_LCV(GLInFmt, GLFormat)
	DERIVED_LCV(GLInSizedFmt, GLFormat)
	DERIVED_LCV(GLInCompressedFmt, GLFormat)
	DERIVED_LCV(GLInRenderFmt, GLFormat)
	DERIVED_LCV(GLInReadFmt, GLFormat)
	DERIVED_LCV(GLTypeFmt, GLFormat)
#undef DEF_LCV
#undef DEF_LCV0
#undef DERIVED_LCV

	// Enum型はlua_Integerに読み替え
	template <class T>
	lua_Integer DetectLCVType(std::true_type);
	template <class T>
	T DetectLCVType(std::false_type);
	template <class T>
	using GetLCVTypeRaw = decltype(DetectLCVType<T>(typename std::is_enum<T>::type()));
	template <class T>
	using GetLCVType = LCV<GetLCVTypeRaw<T>>;

	// ベクトル, 行列, クォータニオン(Aligned)タイプはUnAlignedと同じ扱いにする
	template <int N>
	struct LCV<spn::VecT<N,true>> : LCV<spn::VecT<N,false>> {};
	template <int M, int N>
	struct LCV<spn::MatT<M,N,true>> : LCV<spn::MatT<M,N,false>> {};
	template <class T>
	struct LCV<spn::Optional<T>> {
		int operator()(lua_State* ls, const spn::Optional<T>& op) const {
			if(!op)
				return GetLCVType<LuaNil>()(ls, LuaNil());
			else
				return GetLCVType<T>()(ls, *op);
		}
		spn::Optional<T> operator()(int idx, lua_State* ls) const {
			if(lua_type(ls, idx) == LUA_TNIL)
				return spn::none;
			return GetLCVType<T>()(idx, ls);
		}
		std::ostream& operator()(std::ostream& os, const spn::Optional<T>& t) const {
			if(t)
				return GetLCVType<T>()(os, *t);
			else
				return os << "(none)";
		}
		LuaType operator()() const {
			return GetLCVType<T>()(); }
	};
	template <class T>
	std::ostream& operator << (std::ostream& os, LCV<spn::Optional<T>>) {
		return os << "Optional<" << LCV<T>() << '>';
	}
	// --- LCV<spn::Range<T>> = LCV<std::vector<T>>
	template <class T>
	struct LCV<spn::Range<T>> {
		using Vec_t = std::vector<T>;
		using LCV_t = GetLCVType<Vec_t>;
		int operator()(lua_State* ls, const spn::Range<T>& r) const {
			return LCV_t()(ls, {r.from, r.to}); }
		spn::Range<T> operator()(int idx, lua_State* ls) const {
			auto p = LCV_t()(idx, ls);
			return {p[0], p[1]};
		}
		std::ostream& operator()(std::ostream& os, const spn::Range<T>& r) const {
			return LCV_t()(os, {r.from, r.to}); }
		LuaType operator()() const {
			return LCV_t()(); }
	};
	template <class T>
	std::ostream& operator << (std::ostream& os, LCV<spn::Range<T>>) {
		return os << "Range<" << LCV<T>() << '>';
	}
	// --- LCV<Duration> = LUA_TNUMBER
	template <class Rep, class Period>
	struct LCV<std::chrono::duration<Rep,Period>> {
		using Dur = std::chrono::duration<Rep,Period>;
		int operator()(lua_State* ls, const Dur& d) const {
			return LCV<lua_Integer>()(ls, std::chrono::duration_cast<Microseconds>(d).count()); }
		Dur operator()(int idx, lua_State* ls) const {
			return Microseconds(LCV<lua_Integer>()(idx, ls)); }
		std::ostream& operator()(std::ostream& os, const Dur& d) const {
			return os << d; }
		LuaType operator()() const {
			return LuaType::Number; }
	};
	template <class... Ts>
	std::ostream& operator << (std::ostream& os, LCV<std::chrono::duration<Ts...>>) {
		return os << "duration";
	}

	// ----------- LCV<HdlLock> -----------
	template <class T, bool D>
	struct LCV<spn::HdlLock<T,D>> {
		using Handle_t = spn::HdlLock<T,D>;
		using SH = decltype(std::declval<Handle_t>().get());
		int operator()(lua_State* ls, const Handle_t& t) const {
			return LCV<SH>()(ls, t.get()); }
		Handle_t operator()(int idx, lua_State* ls) const {
			return LCV<SH>()(idx, ls); }
		std::ostream& operator()(std::ostream& os, const Handle_t& t) const {
			return LCV<SH>()(os, t.get()); }
		LuaType operator()() const {
			return LCV<SH>()(); }
	};
	template <class T, bool D>
	std::ostream& operator << (std::ostream& os, LCV<spn::HdlLock<T,D>>) {
		return os << "LHandle";
	}
	// ----------- LCV<VH> -----------
	template <class T>
	struct LCV<spn::VHChk<T>> : LCV<T> {};
	template <class T>
	std::ostream& operator << (std::ostream& os, LCV<spn::VHChk<T>>) {
		return os << T();
	}
	// ----------- LCV<SHandleT> -----------
	template <class... Ts>
	struct LCV<spn::SHandleT<Ts...>> : LCV<spn::SHandle> {
		using Handle_t = spn::SHandleT<Ts...>;
		int operator()(lua_State* ls, const Handle_t& t) const {
			return LCV<spn::SHandle>()(ls, t); }
		Handle_t operator()(int idx, lua_State* ls) const {
			return  Handle_t::FromHandle(LCV<spn::SHandle>()(idx, ls)); }
	};
	template <class T, class... Ts>
	std::ostream& operator << (std::ostream& os, LCV<spn::SHandleT<T, Ts...>>) {
		return os << "SHandle";
	}
	// ----------- LCV<WHandleT> -----------
	template <class... Ts>
	struct LCV<spn::WHandleT<Ts...>> : LCV<spn::WHandle> {
		using Handle_t = spn::WHandleT<Ts...>;
		int operator()(lua_State* ls, const Handle_t& t) const {
			return LCV<spn::WHandle>()(ls, t); }
		Handle_t operator()(int idx, lua_State* ls) const {
			return Handle_t::FromHandle(LCV<spn::WHandle>()(idx, ls)); }
	};
	template <class T, class... Ts>
	std::ostream& operator << (std::ostream& os, LCV<spn::WHandleT<T, Ts...>>) {
		return os << "WHandle";
	}

	template <class T, class A>
	struct LCV<std::vector<T, A>> {
		using Vec_t = std::vector<T,A>;
		int operator()(lua_State* ls, const Vec_t& v) const {
			GetLCVType<T> lcv;
			auto sz = v.size();
			lua_createtable(ls, sz, 0);
			for(std::size_t i=0 ; i<sz ; i++) {
				lua_pushinteger(ls, i+1);
				lcv(ls, v[i]);
				lua_settable(ls, -3);
			}
			return 1;
		}
		Vec_t operator()(int idx, lua_State* ls) const {
			idx = lua_absindex(ls, idx);
			GetLCVType<T> lcv;
			int stk = lua_gettop(ls);
			lua_len(ls, idx);
			int sz = lua_tointeger(ls, -1);
			lua_pop(ls, 1);
			Vec_t ret(sz);
			for(int i=0 ; i<sz ; i++) {
				lua_pushinteger(ls, i+1);
				lua_gettable(ls, idx);
				ret[i] = static_cast<T>(lcv(-1, ls));
				lua_pop(ls, 1);
			}
			lua_settop(ls, stk);
			return ret;
		}
		std::ostream& operator()(std::ostream& os, const Vec_t& v) const {
			return os << "(vector size=" << v.size() << ")"; }
		LuaType operator()() const {
			return LuaType::Table; }
	};
	template <class T, class A>
	std::ostream& operator << (std::ostream& os, LCV<std::vector<T,A>>) {
		return os << "vector<" << LCV<T>() << '>';
	}
	template <class... Ts>
	struct LCV<std::tuple<Ts...>> {
		using Tuple = std::tuple<Ts...>;
		template <std::size_t N>
		using IConst = std::integral_constant<std::size_t, N>;

		// std::tuple<> -> Args...
		int _pushElem(lua_State* /*ls*/, const Tuple& /*t*/, IConst<sizeof...(Ts)>) const {
			return 0; }
		template <std::size_t N>
		int _pushElem(lua_State* ls, const Tuple& t, IConst<N>) const {
			using T = typename std::decay<typename std::tuple_element<N, Tuple>::type>::type;
			int count = GetLCVType<T>()(ls, std::get<N>(t));
			return count + _pushElem(ls, t, IConst<N+1>());
		}
		int operator()(lua_State* ls, const Tuple& t) const {
			return _pushElem(ls, t, IConst<0>());
		}

		// Table -> std::tuple<>
		void _getElem(Tuple& /*dst*/, int /*idx*/, lua_State* /*ls*/, IConst<sizeof...(Ts)>) const {}
		template <std::size_t N>
		void _getElem(Tuple& dst, int idx, lua_State* ls, IConst<N>) const {
			lua_pushinteger(ls, N+1);
			lua_gettable(ls, idx);
			std::get<N>(dst) = GetLCVType<typename std::tuple_element<N, Tuple>::type>()(-1, ls);
			lua_pop(ls, 1);
			_getElem(dst, idx, ls, IConst<N+1>());
		}
		Tuple operator()(int idx, lua_State* ls) const {
			Tuple ret;
			_getElem(ret, idx, ls, IConst<0>());
			return ret;
		}

		std::ostream& operator()(std::ostream& os, const Tuple& /*v*/) const {
			return os << "(tuple size=" << sizeof...(Ts) << ")"; }
		LuaType operator()() const {
			return LuaType::Table; }
	};
	template <class T0, class T1>
	struct LCV<std::pair<T0,T1>> : LCV<std::tuple<T0,T1>> {
		using base_t = LCV<std::tuple<T0,T1>>;
		using Pair = std::pair<T0,T1>;
		using Tuple = std::tuple<T0,T1>;
		Pair operator()(int idx, lua_State* ls) const {
			auto ret = base_t::operator()(idx, ls);
			return {std::get<0>(ret), std::get<1>(ret)};
		}
		using base_t::operator();
	};
	template <class T0, class T1>
	std::ostream& operator << (std::ostream& os, LCV<std::pair<T0,T1>>) {
		return os << "pair<" << LCV<T0>() << ", " << LCV<T1>() << '>';
	}

	//! lua_Stateの単純なラッパークラス
	class LuaState : public std::enable_shared_from_this<LuaState> {
		template <class T>
		friend struct LCV;
		template <int N>
		using IConst = std::integral_constant<int,N>;
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
							ENT_NREF,
							ENT_SPILUA;
			SPLua		_base;		//!< メインスレッド (自身がそれな場合はnull)
			SPILua		_lua;		//!< 自身が保有するスレッド
			int			_id;
			static spn::FreeList<int> s_index;
			static void Nothing(lua_State* ls);
			static void Delete(lua_State* ls);

			// -------------- Exceptions --------------
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
			void _registerNewThread(LuaState& lsc, int id, bool bBase);
			/* Thread変数の参照カウント機構の為の関数群
				(Luaのthread変数には__gcフィールドを設定できないため、このような面倒くさい事になっている) */
			//! global[cs_fromId], global[cs_fromThread]を無ければ作成しスタックに積む
			/*! FromIdはLuaStateのシリアルIdから詳細情報を
				FromThreadはLuaのスレッド変数から参照する物 */
			static void _LoadThreadTable(LuaState& lsc);
			static SPILua _Increment(LuaState& lsc, std::function<void (LuaState&)> cb);
			//! Idをキーとして参照カウンタをインクリメント
			static SPILua _Increment_ID(LuaState& lsc, int id);
			//! スレッド変数をキーとして参照カウンタをインクリメント
			/*! スタックトップにThread変数を積んでから呼ぶ */
			static SPILua _Increment_Th(LuaState& lsc);
			static void _Decrement(LuaState& lsc, std::function<void (LuaState&)> cb);
			static void _Decrement_ID(LuaState& lsc, int id);
			static void _Decrement_Th(LuaState& lsc);
			//! NewThread初期化
			LuaState(const SPLua& spLua);
			//! スレッド共有初期化
			/*! lua_Stateから元のLuaStateクラスを辿り、そのshared_ptrをコピーして使う */
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
			template <class TUP, int N, typename std::enable_if<N==std::tuple_size<TUP>::value>::type*& = spn::Enabler>
			void _popValues(TUP&, int, IConst<N>) {}
			template <class TUP, int N, typename std::enable_if<N!=std::tuple_size<TUP>::value>::type*& = spn::Enabler>
			void _popValues(TUP& dst, int ofs, IConst<N>) {
				std::get<N>(dst) = toValue<typename std::tuple_element<N, TUP>::type>(ofs + N);
				_popValues(dst, ofs, IConst<N+1>());
			}

		public:
			// Luaステートの新規作成
			LuaState(lua_Alloc f=nullptr, void* ud=nullptr);
			LuaState(const LuaState&) = delete;
			LuaState(LuaState&& ls);
			// 借り物初期化 (破棄の処理はしない)
			LuaState(const SPILua& ls);
			LuaState(lua_State* ls);

			//! リソースパスからファイル名指定でスクリプトを読み込み、標準ライブラリもロードする
			static SPLua FromResource(const std::string& name);
			const static char* cs_defaultmode;
			//! Text/Binary形式でLuaソースを読み取り、チャンクをスタックトップに積む
			int load(HRW hRW, const char* chunkName=nullptr, const char* mode=cs_defaultmode, bool bExec=true);
			//! ソースコードを読み取り、チャンクをスタックトップに積む
			/*! \param[in] bExec チャンクを実行するか */
			int loadFromSource(HRW hRW, const char* chunkName=nullptr, bool bExec=true);
			//! コンパイル済みバイナリを読み取り、チャンクをスタックトップに積む
			int loadFromBinary(HRW hRW, const char* chunkName=nullptr, bool bExec=true);
			int loadModule(const std::string& name);
			//! 任意の値をスタックに積む
			void push(const LCValue& v);
			template <class T>
			void push(const LValue<T>& v) {
				v.prepareValue(getLS());
			}
			//! C関数をスタックに積む
			/*! \param[in] nvalue 関連付けるUpValueの数 */
			void pushCClosure(lua_CFunction func, int nvalue);
			//! 任意の複数値をスタックに積む
			template <class A, class... Args>
			void pushArgs(A&& a, Args&&... args) {
				push(std::forward<A>(a));
				pushArgs(std::forward<Args>(args)...);
			}
			void pushArgs() {}
			template <class... Ret>
			void popValues(std::tuple<Ret...>& dst) {
				int ofs = getTop() - sizeof...(Ret);
				AssertP(Trap, ofs>=0, "not enough values on stack (needed: %1%, actual: %2%)", sizeof...(Ret), getTop())
				_popValues(dst, ofs+1, IConst<0>());
				pop(sizeof...(Ret));
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
			int call(int nargs, int nresults=LUA_MULTRET);
			int callk(int nargs, int nresults, lua_KContext ctx, lua_KFunction k);
			bool checkStack(int extra);
			bool compare(int idx1, int idx2, CMP cmp) const;
			void concat(int n);
			void copy(int from, int to);
			void dump(lua_Writer writer, void* data);
			void error();
			int gc(GC what, int data);
			lua_Alloc getAllocf(void** ud) const;
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
			lua_Integer toInteger(int idx) const;
			std::string toString(int idx) const;
			std::string cnvString(int idx);
			lua_Number toNumber(int idx) const;
			const void* toPointer(int idx) const;
			void* toUserData(int idx) const;
			SPLua toThread(int idx) const;
			LCTable toTable(int idx, LPointerSP* spm=nullptr) const;
			LCValue toLCValue(int idx, LPointerSP* spm=nullptr) const;

			template <class R>
			decltype(auto) toValue(int idx) const {
				return GetLCVType<R>()(idx, getLS());
			}

			LuaType type(int idx) const;
			static LuaType SType(lua_State* ls, int idx);
			const char* typeName(LuaType typ) const;
			static const char* STypeName(lua_State* ls, LuaType typ);
			const lua_Number* version() const;
			void xmove(const SPLua& to, int n);
			int yield(int nresults);
			int yieldk(int nresults, lua_KContext ctx, lua_KFunction k);

			/*! スタックトップのテーブルに"name"というテーブルが無ければ作成
				既にあれば単にそれを積む */
			void prepareTable(const std::string& name);
			void prepareTableGlobal(const std::string& name);

			lua_State* getLS() const;
			SPLua getLS_SP();
			SPLua getMainLS_SP();
			static SPLua GetMainLS_SP(lua_State* ls);
			friend std::ostream& operator << (std::ostream& os, const LuaState&);
	};
	using SPLua = std::shared_ptr<LuaState>;
	std::ostream& operator << (std::ostream& os, const LuaState& ls);
	//! デストラクタでLuaスタック位置を元に戻す
	class RewindTop {
		private:
			lua_State	*const	_ls;
			const int			_base;		//!< 初期化された時点でのスタック位置
			bool				_bReset;	//!< trueならスタック位置をbaseへ戻す
		public:
			RewindTop(lua_State* ls);
			void setReset(bool r);
			int getNStack() const;
			~RewindTop();
	};

	template <class T>
	class LValue;
	namespace detail {
		// LValue[], *LValueの時だけ生成される中間クラス
		template <class LV, class IDX>
		class LV_Inter {
			LV&				_src;
			const IDX&		_index;
			public:
				LV_Inter(LV& src, const IDX& index): _src(src), _index(index) {}
				LV_Inter(const LV_Inter&) = delete;
				LV_Inter(LV_Inter&&) = default;

				void prepareValue(lua_State* ls) const {
					_src.prepareAt(ls, _index);
				}
				template <class VAL>
				LV_Inter& operator = (VAL&& v) {
					_src.setField(_index, std::forward<VAL>(v));
					return *this;
				}
				lua_State* getLS() const {
					return _src.getLS();
				}
		};
		// LV_Inter (const版)
		template <class LV, class IDX>
		class LV_Inter<const LV, IDX> : public LV_Inter<LV,IDX> {
			public:
				LV_Inter(const LV& src, const IDX& index):
					LV_Inter<LV,IDX>(const_cast<LV&>(src), index) {}
				LV_Inter(const LV_Inter&) = delete;
				LV_Inter(LV_Inter&&) = default;

				template <class VAL>
				LV_Inter& operator = (VAL&& v) = delete;
		};
		//! LValue内部の値をスタックに積み、デストラクタで元に戻す
		template <class T>
		struct VPop {
			const T&	self;
			int			index;
			VPop(const T& s, const bool bTop):
				self(s)
			{
				index = s._prepareValue(bTop);
			}
			operator int() const { return index; }
			int getIndex() const { return index; }
			~VPop() {
				self._cleanValue(index);
			}
		};
	}
	class LV_Global {
		const static std::string	cs_entry;
		static spn::FreeList<int>	s_index;
		SPLua		_lua;
		int			_id;

		void _init(const SPLua& sp);
		public:
			using VPop = detail::VPop<LV_Global>;
			friend VPop;
		protected:
			// 代入する値をスタックの先頭に置いた状態で呼ぶ
			void _setValue();
			// 当該値をスタックに置く
			int _prepareValue(bool bTop) const;
			void _prepareValue(lua_State* ls) const;
			void _cleanValue(int pos) const;
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
			void swap(LV_Global& lv) noexcept;
			friend std::ostream& operator << (std::ostream& os, const LV_Global& t);
	};
	class LV_Stack {
		lua_State*	_ls;
		int			_pos;
		#ifdef DEBUG
			LuaType	_type;
		#endif

		public:
			using VPop = detail::VPop<LV_Stack>;
			friend VPop;
		protected:
			void _setValue();
			void _init(lua_State* ls);
			int _prepareValue(bool bTop) const;
			void _prepareValue(lua_State* ls) const;
			void _cleanValue(int pos) const;
		public:
			LV_Stack(lua_State* ls);
			LV_Stack(lua_State* ls, const LCValue& lcv);
			template <class LV, class IDX>
			LV_Stack(detail::LV_Inter<LValue<LV>,IDX>&& lv) {
				lua_State* ls = lv.getLS();
				lv.prepareValue(ls);
				_init(ls);
			}
			template <class LV>
			LV_Stack(lua_State* ls, const LValue<LV>& lv) {
				lv.prepareValue(ls);
				_init(ls);
			}
			LV_Stack(const LV_Stack& lv);
			~LV_Stack();

			template <class LV>
			LV_Stack& operator = (const LValue<LV>& lcv) {
				lcv.prepareValue(_ls);
				_setValue();
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
		public:
			template <class Callback>
			void iterateTable(Callback&& cb) const {
				LuaState lsc(T::getLS());
				typename T::VPop vp(*this, true);
				// LValueの値がテーブル以外の時は処理しない
				if(lsc.type(-1) == LuaType::Table) {
					lsc.push(LuaNil());
					while(lsc.next(vp.getIndex()) != 0) {
						cb(lsc);
						lsc.pop(1);
					}
				}
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
			LValue& operator = (LValue&& lv) noexcept {
				static_cast<T&>(*this).swap(static_cast<T&>(lv));
				return *this;
			}
			template <class T2>
			LValue& operator = (T2&& t) {
				static_cast<T&>(*this) = std::forward<T2>(t);
				return *this;
			}
			template <class LV>
			auto operator [](const LValue<LV>& lv) {
				return detail::LV_Inter<LValue<LV>, LValue<LV>>(*this, lv);
			}
			template <class LV>
			auto operator [](const LValue<LV>& lv) const {
				return detail::LV_Inter<const LValue<LV>, LValue<LV>>(*this, lv);
			}
			auto operator [](const LCValue& lcv) {
				return detail::LV_Inter<LValue<T>, LCValue>(*this, lcv);
			}
			auto operator [](const LCValue& lcv) const {
				return detail::LV_Inter<const LValue<T>, LCValue>(*this, lcv);
			}
			template <class... Ret, class... Args>
			void operator()(std::tuple<Ret...>& dst, Args&&... args) {
				LuaState lsc(T::getLS());
				T::_prepareValue(true);
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
				return ret;
			}
			template <class... Args>
			SPLCTable callNRet(Args&&... args) {
				LuaState lsc(T::getLS());
				int top = lsc.getTop();
				T::_prepareValue(true);
				// 引数をスタックに積んで関数コール
				lsc.pushArgs(std::forward<Args>(args)...);
				lsc.call(sizeof...(Args), LUA_MULTRET);
				int nRet = lsc.getTop() - top;
				if(nRet == 0)
					return nullptr;
				// 戻り値はテーブルで返す
				auto spTbl = std::make_shared<LCTable>();
				for(int i=0 ; i<nRet ; i++)
					(*spTbl)[i+1] = lsc.toLCValue(top + i + 1);
				lsc.setTop(top);
				return spTbl;
			}
			template <class... Ret, class... Args>
			std::tuple<Ret...> callMethod(const std::string& method, Args&&... args) {
				LValue lv = (*this)[method];
				return lv.call<Ret...>(*this, std::forward<Args>(args)...);
			}
			template <class... Args>
			LCValue callMethodNRet(const std::string& method, Args&&... args) {
				LValue lv = (*this)[method];
				return lv.callNRet(*this, std::forward<Args>(args)...);
			}

			// --- convert function ---
			#define DEF_FUNC(typ, name) typ name() const { \
				return LCV<typ>()(typename T::VPop(*this, true), T::getLS()); }
			DEF_FUNC(bool, toBoolean)
			DEF_FUNC(lua_Integer, toInteger)
			DEF_FUNC(lua_Number, toNumber)
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
				typename T::VPop vp(*this, true);
				GetLCVType<typename LValue_LCVT<IDX>::type>()(ls, idx);
				GetLCVType<typename LValue_LCVT<VAL>::type>()(ls, val);
				lua_settable(ls, vp.getIndex());
			}

			const void* toPointer() const {
				return lua_topointer(T::getLS(), typename T::VPop(*this, false));
			}
			template <class R>
			decltype(auto) toValue() const {
				return GetLCVType<R>()(typename T::VPop(*this, false), T::getLS());
			}
			int length() const {
				typename T::VPop vp(*this, true);
				auto* ls = T::getLS();
				lua_len(ls, vp.getIndex());
				int ret = lua_tointeger(ls, -1);
				lua_pop(ls, 1);
				return ret;
			}
			LuaType type() const {
				return LuaState::SType(T::getLS(), typename T::VPop(*this, false));
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
		static auto callCB(CB cb, lua_State* /*ls*/, int /*idx*/, Ts1&&... ts1) -> decltype(cb(std::forward<Ts1>(ts1)...)) {
			return cb(std::forward<Ts1>(ts1)...);
		}
		template <class T, class RT, class FT, class... Args, class... Ts1>
		static RT procMethod(lua_State* /*ls*/, T* ptr, int /*idx*/, RT (FT::*func)(Args...), Ts1&&... ts1) {
			return (ptr->*func)(std::forward<Ts1>(ts1)...);
		}
		template <class RT, class... Args, class... Ts1>
		static RT proc(lua_State* /*ls*/, int /*idx*/, RT (*func)(Args...), Ts1&&... ts1) {
			return func(std::forward<Ts1>(ts1)...);
		}
	};

	template <class T>
	using DecayT = typename std::decay<T>::type;
	template <class Ts0A, class... Ts0>
	struct FuncCall<Ts0A, Ts0...> {
		template <class CB, class... Ts1>
		static decltype(auto) callCB(CB&& cb, lua_State* ls, int idx, Ts1&&... ts1) {
			decltype(auto) value = GetLCVType<DecayT<Ts0A>>()(idx, ls);
			return FuncCall<Ts0...>::callCB(std::forward<CB>(cb),
					ls,
					idx+1,
					std::forward<Ts1>(ts1)...,
					(Ts0A)value
					);
		}
		template <class T, class RT, class FT, class... Args, class... Ts1>
		static RT procMethod(lua_State* ls, T* ptr, int idx, RT (FT::*func)(Args...), Ts1&&... ts1) {
			decltype(auto) value = GetLCVType<DecayT<Ts0A>>()(idx, ls);
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
			decltype(auto) value = GetLCVType<DecayT<Ts0A>>()(idx, ls);
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
		template <class CB>
		static int proc(lua_State* ls, CB cb) {
			return GetLCVType<T>()(ls, static_cast<GetLCVTypeRaw<T>>(cb()));
		}
	};
	template <>
	struct RetSize<void> {
		constexpr static int size = 0;
		template <class CB>
		static int proc(lua_State* /*ls*/, CB cb) {
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
		extern const std::string ScriptResourceEntry,
								SystemScriptResourceEntry,
								ScriptExtension;
		extern const std::string Udata,
								Pointer,
								ToString;
		extern const std::string GetInstance,
								DeleteHandle,
								SetState,
								SwitchState,
								Null,
								ObjectBase,
								ConstructPtr,
								DerivedHandle,
								MakeFSMachine,
								FSMachine,
								MakePreENV,
								Ctor,
								RecvMsg,
								RecvMsgCpp,
								OnUpdate,
								OnPause,
								OnEffectReset,
								OnResume,
								OnExit,
								System;
		namespace objBase {
			extern const std::string ValueR,
									ValueW,
									Func,
									UdataMT,
									MT,
									ClassName,
									_Pointer,
									_New;
			namespace valueR {
				extern const std::string HandleName,
										HandleId,
										NumRef;
			}
		}
		namespace system {
			extern const std::string PathSeparation,
									PathReplaceMark,
									Package,
									Path;
		}
	}
}
DEF_LUAIMPORT_BASE
namespace rs {
	// --- Lua->C++グルーコードにおけるクラスポインタの取得方法 ---
	//! "pointer"に生ポインタが記録されている
	struct LI_GetPtrBase {
		void* operator()(lua_State* ls, int idx, const char* name) const;
	};
	struct LI_GetHandleBase {
		void* operator()(lua_State* ls, int idx, const char* name) const;
		spn::SHandle getHandle(lua_State* ls, int idx) const;
	};
	template <class T>
	struct LI_GetPtr : LI_GetPtrBase {
		T* operator()(lua_State* ls, int idx) const {
			return reinterpret_cast<T*>(LI_GetPtrBase()(ls, idx, lua::LuaName((T*)nullptr)));
		}
	};
	//! "udata"にハンドルが記録されている -> void*からそのままポインタ変換
	template <class T>
	struct LI_GetHandle : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			return reinterpret_cast<T*>(LI_GetHandleBase::operator()(ls, idx, lua::LuaName((T*)nullptr))); }
	};
	//! "udata"にハンドルが記録されている -> unique_ptrからポインタ変換
	template <class T, class Deleter>
	struct LI_GetHandle<std::unique_ptr<T, Deleter>> : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			auto* up = reinterpret_cast<std::unique_ptr<T,Deleter>*>(
				LI_GetHandleBase::operator()(ls, idx, lua::LuaName((T*)nullptr)));
			return up->get();
		}
	};
	//! "udata"にハンドルが記録されている -> shared_ptrからポインタ変換
	template <class T>
	struct LI_GetHandle<std::shared_ptr<T>> : LI_GetHandleBase {
		T* operator()(lua_State* ls, int idx) const {
			auto* up = reinterpret_cast<std::shared_ptr<T>*>(
				LI_GetHandleBase::operator()(ls, idx, lua::LuaName((T*)nullptr)));
			return up->get();
		}
	};
	namespace lua {
		template <>
		const char* LuaName(spn::SHandle*);
		template <>
		const char* LuaName(IGLResource*);
	}
	//! LuaへC++のクラスをインポート、管理する
	class LuaImport {
		//! インポートしたクラス、関数や変数をテキスト出力(デバッグ用)
		static std::stringstream	s_importLog;
		static std::string			s_firstBlock;
		static int					s_indent;
		using LogMap = std::map<std::string, std::string>;
		static LogMap				s_logMap;

		//! ハンドルオブジェクトの基本メソッド
		static const std::string& HandleName(spn::SHandle sh);
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

		static std::ostream& _PushIndent(std::ostream& s);
		template <class... Ts, class=typename std::enable_if<sizeof...(Ts)==0>::type>
		static std::ostream& _OutputArgs(std::ostream& s, bool=true) { return s; }
		template <class T, class... Ts>
		static std::ostream& _OutputArgs(std::ostream& s, bool bFirst=true) {
			if(!bFirst)
				s << ", ";
			s << LCV<T>();
			return _OutputArgs<Ts...>(s, false);
		}
		public:
			static void BeginImportBlock(const std::string& name);
			static void EndImportBlock();
			static void SaveImportLog(const std::string& path);

			//! lscにFuncTableを積んだ状態で呼ぶ
			template <class GET, class T, class RT, class FT, class... Ts>
			static void RegisterMember(LuaState& lsc, const char* name, RT (FT::*func)(Ts...)) {
				lsc.push(name);
				SetMethod(lsc.getLS(), func);
				lsc.pushCClosure(&CallMethod<GET,T,RT,FT,Ts...>, 1);
				lsc.rawSet(-3);

				// ---- ログ出力 ----
				_PushIndent(s_importLog) << LCV<RT>() << ' ' << name << '(';
				_OutputArgs<Ts...>(s_importLog) << ')' << std::endl;
			}
			template <class GET, class T, class RT, class FT, class... Ts>
			static void RegisterMember(LuaState& lsc, const char* name, RT (FT::*func)(Ts...) const) {
				RegisterMember<GET,T>(lsc, name, (RT (FT::*)(Ts...))func);
			}
			//! lscにReadTable, WriteTableを積んだ状態で呼ぶ
			template <class GET, class T, class V, class VT>
			static void RegisterMember(LuaState& lsc, const char* name, V VT::*member) {
				// ReadValue関数の登録
				lsc.push(name);
				SetMember(lsc.getLS(), member);
				lsc.pushCClosure(&ReadValue<GET,T,V,VT>, 1);
				lsc.rawSet(-4);
				// WriteValue関数の登録
				lsc.push(name);
				SetMember(lsc.getLS(), member);
				lsc.pushCClosure(&WriteValue<GET,T,V,VT>, 1);
				lsc.rawSet(-3);

				// ---- ログ出力 ----
				_PushIndent(s_importLog) << LCV<V>() << ' ' << name << std::endl;
			}
			//! static なメンバ関数はCFunctionとして登録
			template <class GET, class T, class RT, class... Ts>
			static void RegisterMember(LuaState& lsc, const char* name, RT (*func)(Ts...)) {
				lsc.prepareTableGlobal(lua::LuaName((T*)nullptr));
				lsc.push(name);
				PushFunction(lsc, func);
				lsc.rawSet(-3);
				lsc.pop(1);

				// ---- ログ出力 ----
				_PushIndent(s_importLog) << "static " << LCV<RT>() <<  ' ' << name << '(';
				_OutputArgs<Ts...>(s_importLog) << ')' << std::endl;
			}
			static int ReturnException(lua_State* ls, const char* func, const std::exception& e, int nNeed);
			//! luaスタックから変数ポインタとクラスを取り出しメンバ変数を読み込む
			template <class GET, class T, class V, class VT>
			static int ReadValue(lua_State* ls) {
				try {
					// up[1]	変数ポインタ
					// [1]		クラスポインタ(userdata)
					const T* src = reinterpret_cast<const T*>(GET()(ls, 1));
					auto vp = GetMember<V,VT>(ls, lua_upvalueindex(1));
					GetLCVType<V>()(ls, src->*vp);
					return 1;
				} catch(const std::exception& e) {
					return ReturnException(ls, __PRETTY_FUNCTION__, e, 1);
				}
			}
			//! luaスタックから変数ポインタとクラスと値を取り出しメンバ変数に書き込む
			template <class GET, class T, class V, class VT>
			static int WriteValue(lua_State* ls) {
				try {
					// up[1]	変数ポインタ
					// [1]		クラスポインタ(userdata)
					// [2]		セットする値
					T* dst = reinterpret_cast<T*>(GET()(ls, 1));
					auto ptr = GetMember<V,VT>(ls, lua_upvalueindex(1));
					(dst->*ptr) = GetLCVType<V>()(2, ls);
					return 0;
				} catch(const std::exception& e) {
					return ReturnException(ls, __PRETTY_FUNCTION__, e, 2);
				}
			}
			//! luaスタックから関数ポインタとクラス、引数を取り出しクラスのメンバ関数を呼ぶ
			template <class GET, class T, class RT, class FT, class... Args>
			static int CallMethod(lua_State* ls) {
				try {
					// up[1]	関数ポインタ
					// [1]		クラスポインタ(userdata)
					// [2以降]	引数
					using F = RT (FT::*)(Args...);
					void* tmp = lua_touserdata(ls, lua_upvalueindex(1));
					F f = *reinterpret_cast<F*>(tmp);
					auto* ptr = dynamic_cast<T*>(GET()(ls, 1));
					return RetSize<RT>::proc(ls, [ls,ptr,f]() -> decltype(auto) { return FuncCall<Args...>::procMethod(ls, ptr, 2, f); });
				} catch(const std::exception& e) {
					return ReturnException(ls, __PRETTY_FUNCTION__, e, sizeof...(Args)+1);
				}
			}
			//! luaスタックから関数ポインタと引数を取り出しcall
			template <class RT, class... Args>
			static int CallFunction(lua_State* ls) {
				try {
					// up[1]	関数ポインタ
					// [1以降]	引数
					using F = RT (*)(Args...);
					F f = reinterpret_cast<F>(lua_touserdata(ls, lua_upvalueindex(1)));
					// 引数を変換しつつ関数を呼んで、戻り値を変換しつつ個数を返す
					return RetSize<RT>::proc(ls, [ls,f]() -> decltype(auto) { return FuncCall<Args...>::proc(ls, 1, f); });
				} catch(const std::exception& e) {
					return ReturnException(ls, __PRETTY_FUNCTION__, e, sizeof...(Args));
				}
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

				BeginImportBlock("--Global--");
				// ---- ログ出力 ----
				_PushIndent(s_importLog) << LCV<RT>() <<  ' ' << name << '(';
				_OutputArgs<Ts...>(s_importLog) << ')' << std::endl;
				EndImportBlock();
			}
			//! C++クラスの登録(登録名はクラスから取得)
			template <class T>
			static void RegisterClass(LuaState& lsc) {
				RegisterObjectBase(lsc);
				auto* ptr = static_cast<T*>(nullptr);
				AddObjectId(lua::LuaName(ptr));
				lua::LuaExport(lsc, ptr);
			}
			static void AddObjectId(const char* name);
			static bool IsObjectBaseRegistered(LuaState& lsc);
			static bool IsUpdaterObjectRegistered(LuaState& lsc);
			//! C++クラス登録基盤を初期化
			static void RegisterObjectBase(LuaState& lsc);
			static void RegisterUpdaterObject(LuaState& lsc);
			//! ベースオブジェクトを使った派生クラスの読み込み
			/*! 1クラス1ファイルの対応
				ベースクラスの名前はファイルに記載 */
			static void LoadClass(LuaState& lsc, const std::string& name, HRW hRW);
			//! 固有オブジェクトのインポート
			/*! ポインタ指定でLuaにクラスを取り込む
				リソースマネージャやシステムクラス用 */
			template <class T>
			static void ImportClass(LuaState& lsc, const std::string& tableName, const std::string& name, T* ptr) {
				RegisterObjectBase(lsc);

				int stk = lsc.getTop();
				auto* dummy = static_cast<T*>(nullptr);
				lua::LuaExport(lsc, dummy);
				if(ptr) {
					MakePointerInstance(lsc, lua::LuaName(dummy), ptr);
					// [Instance]
					if(!tableName.empty())
						lsc.prepareTableGlobal(tableName);
					else
						lsc.pushGlobal();
					lsc.push(name);
					// [Instance][Target][name]
					lsc.pushValue(-3);
					// [Instance][Target][name][Instance]
					lsc.rawSet(-3);
				}
				lsc.setTop(stk);
			}
			static void MakePointerInstance(LuaState& lsc, const std::string& luaName, void* ptr);
			template <class T>
			static void MakePointerInstance(LuaState& lsc, const T* ptr) {
				MakePointerInstance(lsc, lua::LuaName((T*)nullptr), (void*)ptr);
			}
			static void RegisterRSClass(LuaState& lsc);
			//! GObjectメッセージを受信
			/*! Obj(UData), MessageStr, {Args} */
			static int RecvMsgCpp(lua_State* ls);
	};
	template <class T>
	struct LCVRaw {
		template <class T2>
		int operator()(lua_State* ls, T2&& t) const {
			LuaState lsc(ls);
			lsc.getGlobal(lua::LuaName((T*)nullptr));
			lsc.getField(-1, luaNS::ConstructPtr);
			lsc.newUserData(sizeof(T));
			new(lsc.toUserData(-1)) T(std::forward<T2>(t));
			lsc.call(1, 1);
			lsc.remove(-2);
			return 1;
		}
		// (int, lua_State*)の順なのは、pushの時と引数が被ってしまう為
		T operator()(int idx, lua_State* ls, LPointerSP* /*spm*/=nullptr) const {
			return *LI_GetPtr<T>()(ls, idx); }
		std::ostream& operator()(std::ostream& os, const T& t) const {
			return os << "(userdata) 0x" << std::hex << &t; }
		LuaType operator()() const {
			return LuaType::Userdata; }
	};
	template <class T>
	struct LCV : LCVRaw<T> {};
	// 参照またはポインターの場合はUserdataに格納
	template <class T>
	struct LCV<const T&> : LCV<T> {};
	template <class T>
	struct LCV<const T*> : LCV<T> {
		using base_t = LCV<T>;
		using base_t::operator();
		int operator()(lua_State* ls, const T* t) const {
			return base_t()(ls, *t); }
		std::ostream& operator()(std::ostream& os, const T* t) const {
			return base_t()(os, *t); }
	};
	template struct LCV<spn::QuatT<false>>;
	template <>
	struct LCV<spn::QuatT<true>> : LCV<spn::QuatT<false>> {};

	// Angleは内部数値をすべてfloatに変換
	template <class Ang, class Rep>
	struct LCV<spn::Angle<Ang, Rep>> : LCV<spn::Angle<Ang,float>> {};

	// 非const参照またはポインターの場合はLightUserdataに格納
	template <class T>
	struct LCV<T*> {
		int operator()(lua_State* ls, const T* t) const {
			LuaState lsc(ls);
			LuaImport::MakePointerInstance(lsc, t);
			return 1; }
		T* operator()(int idx, lua_State* ls, LPointerSP* /*spm*/=nullptr) const {
			return LI_GetPtr<T>()(ls, idx); }
		std::ostream& operator()(std::ostream& os, const T* t) const {
			return LCV<T>()(os, *t); }
		LuaType operator()() const {
			return LuaType::LightUserdata; }
	};
	template <class T>
	struct LCV<T&> : LCV<T*> {
		using base_t = LCV<T*>;
		using base_t::operator();
		int operator()(lua_State* ls, const T& t) const {
			return base_t()(ls, &t); }
		T& operator()(int idx, lua_State* ls, LPointerSP* spm=nullptr) const {
			return *base_t()(idx, ls, spm); }
		std::ostream& operator()(std::ostream& os, const T& t) const {
			return base_t()(os, &t); }
	};

	template <class T,
			 typename std::enable_if<!std::is_pointer<T>::value &&
											!std::is_reference<T>::value &&
											!std::is_unsigned<T>::value>::type*& = spn::Enabler
			>
	std::ostream& operator << (std::ostream& os, const LCV<T>&) { return os << typeid(T).name(); }
	template <class T,
			 typename std::enable_if<std::is_unsigned<T>::value>::type*& = spn::Enabler>
	std::ostream& operator << (std::ostream& os, const LCV<T>&) { return os << "unsigned " << LCV<typename std::make_signed<T>::type>(); }
	template <class T>
	std::ostream& operator << (std::ostream& os, const LCV<const T>&) { return os << "const " << LCV<T>(); }
	template <class T>
	std::ostream& operator << (std::ostream& os, const LCV<T*>&) { return os << LCV<T>() << '*'; }
	template <class T>
	std::ostream& operator << (std::ostream& os, const LCV<T&>&) { return os << LCV<T>() << '&'; }
	template <class T>
	std::ostream& operator << (std::ostream& os, const LCV<T&&>&) { return os << LCV<T>() << "&&"; }
}
