#include "luaw.hpp"
#include <sstream>
#include <limits>
#include "spinner/emplace.hpp"
#include "spinner/size.hpp"

namespace rs {
	bool LuaNil::operator == (LuaNil) const {
		return true;
	}
	std::ostream& operator << (std::ostream& os, const LCTable& lct) {
		return LCV<LCTable>()(os, lct);
	}
	DEF_LCV_OSTREAM(void)
	DEF_LCV_OSTREAM2(lua_OtherNumber, float)
	DEF_LCV_OSTREAM2(lua_OtherInteger, int)
	DEF_LCV_OSTREAM2(spn::Pose2D, Pose2D)
	DEF_LCV_OSTREAM2(spn::Pose3D, Pose3D)
	// ------------------- LCValue -------------------
	// --- LCV<boost::blank> = LUA_TNONE
	int LCV<boost::blank>::operator()(lua_State* /*ls*/, boost::blank) const {
		Assert(Trap, false);
		return 0; }
	boost::blank LCV<boost::blank>::operator()(int /*idx*/, lua_State* /*ls*/, LPointerSP* /*spm*/) const {
		Assert(Trap, false); throw 0; }
	std::ostream& LCV<boost::blank>::operator()(std::ostream& os, boost::blank) const {
		return os << "(none)"; }
	LuaType LCV<boost::blank>::operator()() const {
		return LuaType::LNone; }
	DEF_LCV_OSTREAM(boost::blank)

	// --- LCV<LuaNil> = LUA_TNIL
	int LCV<LuaNil>::operator()(lua_State* ls, LuaNil) const {
		lua_pushnil(ls);
		return 1; }
	LuaNil LCV<LuaNil>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Nil);
		return LuaNil(); }
	std::ostream& LCV<LuaNil>::operator()(std::ostream& os, LuaNil) const {
		return os << "(nil)"; }
	LuaType LCV<LuaNil>::operator()() const {
		return LuaType::Nil; }
	DEF_LCV_OSTREAM(LuaNil)

	// --- LCV<bool> = LUA_TBOOL
	int LCV<bool>::operator()(lua_State* ls, bool b) const {
		lua_pushboolean(ls, b);
		return 1; }
	bool LCV<bool>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Boolean);
		return lua_toboolean(ls, idx) != 0; }
	std::ostream& LCV<bool>::operator()(std::ostream& os, bool b) const {
		return os << std::boolalpha << b; }
	LuaType LCV<bool>::operator()() const {
		return LuaType::Boolean; }
	DEF_LCV_OSTREAM(bool)

	// --- LCV<const char*> = LUA_TSTRING
	int LCV<const char*>::operator()(lua_State* ls, const char* c) const {
		lua_pushstring(ls, c);
		return 1; }
	const char* LCV<const char*>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		return lua_tostring(ls, idx); }
	std::ostream& LCV<const char*>::operator()(std::ostream& os, const char* c) const {
		return os << c; }
	LuaType LCV<const char*>::operator()() const {
		return LuaType::String; }
	DEF_LCV_OSTREAM(const char*)

	// --- LCV<std::string> = LUA_TSTRING
	int LCV<std::string>::operator()(lua_State* ls, const std::string& s) const {
		lua_pushlstring(ls, s.c_str(), s.length());
		return 1; }
	std::string LCV<std::string>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		size_t len;
		const char* c =lua_tolstring(ls, idx, &len);
		return std::string(c, len); }
	std::ostream& LCV<std::string>::operator()(std::ostream& os, const std::string& s) const {
		return os << s; }
	LuaType LCV<std::string>::operator()() const {
		return LuaType::String; }
	DEF_LCV_OSTREAM(std::string)

	namespace {
		auto GetAngleType(lua_State* ls, int idx) {
			LuaState lsc(ls);
			lsc.getField(idx, luaNS::objBase::ClassName);
			auto cname = lsc.toString(-1);
			lsc.pop();
			return cname;
		}
	}
	// --- LCV<spn::DegF>
	int LCV<spn::DegF>::operator()(lua_State* ls, const spn::DegF& d) const {
		return LCVRaw<spn::DegF>()(ls, d); }
	spn::DegF LCV<spn::DegF>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		auto cname = GetAngleType(ls, idx);
		if(cname == "Radian") {
			// Degreeに変換して返す
			auto rad = LCVRaw<spn::RadF>()(idx, ls, spm);
			return spn::DegF(rad);
		} else {
			Assert(Trap, cname=="Degree", "invalid angle type (required Degree or Radian, but got %1%", cname)
			return LCVRaw<spn::DegF>()(idx, ls, spm);
		}
	}
	std::ostream& LCV<spn::DegF>::operator()(std::ostream& os, const spn::DegF& d) const {
		return os << d; }
	LuaType LCV<spn::DegF>::operator()() const {
		return LuaType::Userdata; }
	DEF_LCV_OSTREAM2(spn::DegF, Degree)

	// --- LCV<spn::RadF>
	int LCV<spn::RadF>::operator()(lua_State* ls, const spn::RadF& d) const {
		return LCVRaw<spn::RadF>()(ls, d); }
	spn::RadF LCV<spn::RadF>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		auto cname = GetAngleType(ls, idx);
		if(cname == "Degree") {
			// Radianに変換して返す
			auto rad = LCVRaw<spn::DegF>()(idx, ls, spm);
			return spn::RadF(rad);
		} else {
			Assert(Trap, cname=="Radian", "invalid angle type (required Degree or Radian, but got %1%", cname)
			return LCVRaw<spn::RadF>()(idx, ls, spm);
		}
	}
	std::ostream& LCV<spn::RadF>::operator()(std::ostream& os, const spn::RadF& r) const {
		return os << r; }
	LuaType LCV<spn::RadF>::operator()() const {
		return LuaType::Userdata; }
	DEF_LCV_OSTREAM2(spn::RadF, Radian)

	// --- LCV<lua_Integer> = LUA_TNUMBER
	int LCV<lua_Integer>::operator()(lua_State* ls, lua_Integer i) const {
		lua_pushinteger(ls, i);
		return 1; }
	lua_Integer LCV<lua_Integer>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tointeger(ls, idx); }
	std::ostream& LCV<lua_Integer>::operator()(std::ostream& os, lua_Integer i) const {
		return os << i; }
	LuaType LCV<lua_Integer>::operator()() const {
		return LuaType::Number; }
	DEF_LCV_OSTREAM2(lua_Integer, int)

	// --- LCV<lua_Number> = LUA_TNUMBER
	int LCV<lua_Number>::operator()(lua_State* ls, lua_Number f) const {
		lua_pushnumber(ls, f);
		return 1; }
	lua_Number LCV<lua_Number>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tonumber(ls, idx); }
	std::ostream& LCV<lua_Number>::operator()(std::ostream& os, lua_Number f) const {
		return os << f; }
	LuaType LCV<lua_Number>::operator()() const {
		return LuaType::Number; }
	DEF_LCV_OSTREAM2(lua_Number, float)

	// --- LCV<lua_State*> = LUA_TTHREAD
	int LCV<lua_State*>::operator()(lua_State* ls, lua_State* lsp) const {
		if(ls == lsp)
			lua_pushthread(ls);
		else {
			lua_pushthread(lsp);
			lua_xmove(lsp, ls, 1);
		}
		return 1;
	}
	lua_State* LCV<lua_State*>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		return lua_tothread(ls, idx);
	}
	std::ostream& LCV<lua_State*>::operator()(std::ostream& os, lua_State* ls) const {
		return os << "(thread) " << (uintptr_t)ls;
	}
	LuaType LCV<lua_State*>::operator()() const {
		return LuaType::Thread; }
	DEF_LCV_OSTREAM(lua_State*)

	// --- LCV<spn::RectF> = LUA_TTABLE
	int LCV<spn::RectF>::operator()(lua_State* ls, const spn::RectF& r) const {
		lua_createtable(ls, 4, 0);
		int idx = 1;
		auto fnSet = [ls, &idx](float f){
			lua_pushinteger(ls, idx++);
			lua_pushnumber(ls, f);
			lua_settable(ls, -3);
		};
		fnSet(r.x0);
		fnSet(r.x1);
		fnSet(r.y0);
		fnSet(r.y1);
		return 1;
	}
	spn::RectF LCV<spn::RectF>::operator()(int idx, lua_State* ls, LPointerSP*) const {
		auto fnGet = [ls, idx](int n){
			lua_pushinteger(ls, n);
			lua_gettable(ls, idx);
			auto ret = lua_tonumber(ls, -1);
			lua_pop(ls, 1);
			return ret;
		};
		return {fnGet(1), fnGet(2), fnGet(3), fnGet(4)};
	}
	std::ostream& LCV<spn::RectF>::operator()(std::ostream& os, const spn::RectF& r) const {
		return os << r; }
	LuaType LCV<spn::RectF>::operator()() const {
		return LuaType::Table; }
	DEF_LCV_OSTREAM2(spn::RectF, Rect)

	// --- LCV<spn::Size> = LUA_TTABLE
	int LCV<spn::SizeF>::operator()(lua_State* ls, const spn::SizeF& s) const {
		lua_createtable(ls, 2, 0);
		lua_pushinteger(ls, 1);
		lua_pushnumber(ls, s.width);
		lua_settable(ls, -3);
		lua_pushinteger(ls, 2);
		lua_pushnumber(ls, s.height);
		lua_settable(ls, -3);
		return 1;
	}
	spn::SizeF LCV<spn::SizeF>::operator()(int idx, lua_State* ls, LPointerSP*) const {
		auto fnGet = [ls, idx](int n){
			lua_pushinteger(ls, n);
			lua_gettable(ls, idx);
			auto ret = lua_tonumber(ls, -1);
			lua_pop(ls, 1);
			return ret;
		};
		return {fnGet(1), fnGet(2)};
	}
	std::ostream& LCV<spn::SizeF>::operator()(std::ostream& os, const spn::SizeF& s) const {
		return os << s.width;
	}
	LuaType LCV<spn::SizeF>::operator()() const {
		return LuaType::Table; }
	DEF_LCV_OSTREAM2(spn::SizeF, Size)

	// --- LCV<SPLua> = LUA_TTHREAD
	int LCV<SPLua>::operator()(lua_State* ls, const SPLua& sp) const {
		sp->pushSelf();
		lua_xmove(sp->getLS(), ls, 1);
		return 1;
	}
	SPLua LCV<SPLua>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		auto typ = lua_type(ls, idx);
		if(typ == LUA_TTHREAD)
			return SPLua(new LuaState(lua_tothread(ls, idx), LuaState::TagThread));
		if(typ == LUA_TNIL || typ == LUA_TNONE) {
			// 自身を返す
			return SPLua(new LuaState(ls, LuaState::TagThread));
		}
		LuaState::_CheckType(ls, idx, LuaType::Thread);
		return SPLua();
	}
	std::ostream& LCV<SPLua>::operator()(std::ostream& os, const SPLua& /*sp*/) const {
		return os;
	}
	LuaType LCV<SPLua>::operator()() const {
		return LuaType::Thread; }
	DEF_LCV_OSTREAM(SPLua)

	// --- LCV<void*> = LUA_TLIGHTUSERDATA
	int LCV<void*>::operator()(lua_State* ls, const void* ud) const {
		lua_pushlightuserdata(ls, const_cast<void*>(ud));
		return 1; }
	void* LCV<void*>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		try {
			LuaState::_CheckType(ls, idx, LuaType::Userdata);
		} catch(const LuaState::EType& e) {
			LuaState::_CheckType(ls, idx, LuaType::LightUserdata);
		}
		return lua_touserdata(ls, idx); }
	std::ostream& LCV<void*>::operator()(std::ostream& os, const void* ud) const {
		return os << "(userdata)" << std::hex << ud; }
	LuaType LCV<void*>::operator()() const {
		return LuaType::LightUserdata; }
	DEF_LCV_OSTREAM(void*)

	// --- LCV<lua_CFunction> = LUA_TFUNCTION
	int LCV<lua_CFunction>::operator()(lua_State* ls, lua_CFunction f) const {
		lua_pushcclosure(ls, f, 0);
		return 1; }
	lua_CFunction LCV<lua_CFunction>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Function);
		return lua_tocfunction(ls, idx);
	}
	std::ostream& LCV<lua_CFunction>::operator()(std::ostream& os, lua_CFunction f) const {
		return os << "(function)" << std::hex << reinterpret_cast<uintptr_t>(f); }
	LuaType LCV<lua_CFunction>::operator()() const {
		return LuaType::Function; }
	DEF_LCV_OSTREAM(lua_CFunction)

	// --- LCV<Timepoint> = LUA_TNUMBER
	int LCV<Timepoint>::operator()(lua_State* ls, const Timepoint& t) const {
		LCV<Duration> dur;
		dur(ls, Duration(t.time_since_epoch()));
		return 1;
	}
	Timepoint LCV<Timepoint>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LCV<Duration> dur;
		return Timepoint(dur(idx, ls));
	}
	std::ostream& LCV<Timepoint>::operator()(std::ostream& os, const Timepoint& t) const {
		return os << t; }
	LuaType LCV<Timepoint>::operator()() const {
		return LuaType::Number; }
	DEF_LCV_OSTREAM(Timepoint)

	// --- LCV<LCTable> = LUA_TTABLE
	int LCV<LCTable>::operator()(lua_State* ls, const LCTable& t) const {
		LuaState lsc(ls);
		lsc.newTable(0, t.size());
		for(auto& ent : t) {
			lsc.setField(-1, ent.first, ent.second);
		}
		return 1;
	}
	SPLCTable LCV<LCTable>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Table);

		spn::Optional<LPointerSP> opSet;
		if(!spm) {
			opSet = spn::construct();
			spm = &(*opSet);
		}
		LuaState lsc(ls);
		const void* ptr = lsc.toPointer(idx);
		auto itr = spm->find(ptr);
		if(itr != spm->end())
			return boost::get<SPLCTable>(itr->second);

		// 循環参照対策で先にエントリを作っておく
		SPLCTable ret(new LCTable());
		spn::TryEmplace(*spm, ptr, ret);
		idx = lsc.absIndex(idx);
		lsc.push(LuaNil());
		while(lsc.next(idx) != 0) {
			// key=-2 value=-1
			spn::TryEmplace(*ret, lsc.toLCValue(-2, spm), lsc.toLCValue(-1, spm));
			// valueは取り除きkeyはlua_nextのために保持
			lsc.pop(1);
		}
		return ret;
	}
	//TODO: アドレスを出力しても他のテーブルの区別がつかずあまり意味がないので改善する(出力階層の制限した上で列挙など)
	std::ostream& LCV<LCTable>::operator()(std::ostream& os, const LCTable& t) const {
		return os << "(table)" << std::hex << reinterpret_cast<uintptr_t>(&t); }
	LuaType LCV<LCTable>::operator()() const {
		return LuaType::Table; }
	DEF_LCV_OSTREAM(LCTable)

	// --- LCV<LCValue>
	namespace {
		const std::function<LCValue (lua_State* ls, int idx, LPointerSP* spm)> c_toLCValue[LUA_NUMTAGS+1] = {
			[](lua_State* /*ls*/, int /*idx*/, LPointerSP* /*spm*/){ return LCValue(boost::blank()); },
			[](lua_State* /*ls*/, int /*idx*/, LPointerSP* /*spm*/){ return LCValue(LuaNil()); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<bool>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<void*>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<lua_Number>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<const char*>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<LCTable>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<lua_CFunction>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<void*>()(idx,ls,spm)); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LCV<SPLua>()(idx,ls,spm)); }
		};
	}
	int LCV<LCValue>::operator()(lua_State* ls, const LCValue& lcv) const {
		lcv.push(ls);
		return 1; }
	LCValue LCV<LCValue>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		auto typ = lua_type(ls, idx);
		// Tableにおいて、_prefixフィールド値がVならば_sizeフィールドを読み込みVecTに変換
		if(typ == LUA_TTABLE) {
			lua_pushvalue(ls, idx);
			LValueS lvs(ls);
			LValueS postfix = lvs["_postfix"];
			if(postfix.type() == LuaType::String &&
				std::string(postfix.toString()) == "V")
			{
				int size = LValueS(lvs["_size"]).toInteger();
				const void* ptr = LValueS(lvs["pointer"]).toUserData();
				switch(size) {
					case 2:
						return *static_cast<const spn::Vec2*>(ptr);
					case 3:
						return *static_cast<const spn::Vec3*>(ptr);
					case 4:
						return *static_cast<const spn::Vec4*>(ptr);
				}
				Assert(Trap, false, "invalid vector size (%1%)", size)
			}
		}
		return c_toLCValue[typ+1](ls, idx, spm);
	}
	std::ostream& LCV<LCValue>::operator()(std::ostream& os, const LCValue& lcv) const {
		return os << lcv; }
	LuaType LCV<LCValue>::operator()() const {
		return LuaType::LNone; }
	DEF_LCV_OSTREAM(LCValue)

	// --- LCV<LValueS>
	int LCV<LValueS>::operator()(lua_State* ls, const LValueS& t) const {
		t.prepareValue(ls);
		return 1;
	}
	LValueS LCV<LValueS>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		lua_pushvalue(ls, idx);
		return LValueS(ls);
	}
	std::ostream& LCV<LValueS>::operator()(std::ostream& os, const LValueS& t) const {
		return os << t; }
	LuaType LCV<LValueS>::operator()() const {
		return LuaType::LNone; }
	DEF_LCV_OSTREAM(LValueS)

	// --- LCV<LValueG>
	int LCV<LValueG>::operator()(lua_State* ls, const LValueG& t) const {
		t.prepareValue(ls);
		return 1;
	}
	LValueG LCV<LValueG>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		lua_pushvalue(ls, idx);
		SPLua sp = LuaState::GetMainLS_SP(ls);
		return LValueG(sp);
	}
	std::ostream& LCV<LValueG>::operator()(std::ostream& os, const LValueG& t) const {
		return os << t; }
	LuaType LCV<LValueG>::operator()() const {
		return LuaType::LNone; }
	DEF_LCV_OSTREAM(LValueG)

	namespace {
		struct Visitor : boost::static_visitor<> {
			lua_State* _ls;
			Visitor(lua_State* ls): _ls(ls) {}
			template <class T>
			void operator()(const T& t) const {
				LCV<T>()(_ls, t); }
			void operator()(const SPLua& sp) const {
				LCV<SPLua>()(_ls, sp); }
			template <class T>
			void operator()(const std::shared_ptr<T>& sp) const {
				LCV<T>()(_ls, *sp); }
		};
		struct TypeVisitor : boost::static_visitor<LuaType> {
			template <class T>
			LuaType operator()(const T&) const {
				return LCV<T>()(); }
			LuaType operator()(const SPLua& /*sp*/) const {
				return LCV<SPLua>()(); }
			template <class T>
			LuaType operator()(const std::shared_ptr<T>& /*sp*/) const {
				return LCV<T>()(); }
		};
		struct PrintVisitor : boost::static_visitor<std::ostream*> {
			std::ostream* _os;
			PrintVisitor(std::ostream& os): _os(&os) {}
			template <class T>
			std::ostream* operator()(const T& t) const {
				return &LCV<T>()(*_os, t); }
			std::ostream* operator()(const SPLua& sp) const {
				return &LCV<SPLua>()(*_os, sp); }
			template <class T>
			std::ostream* operator()(const std::shared_ptr<T>& sp) const {
				return &LCV<T>()(*_os, *sp); }
		};

		using spn::SHandle;
		using spn::WHandle;
		using spn::LHandle;
		/*! LHandle -> SHandle
			SHandle -> SHandle */
		struct GetSHVisitor : boost::static_visitor<SHandle> {
			SHandle operator()(const LHandle& l) const { return l.get(); }
			SHandle operator()(SHandle s) const { return s; }
			template <class T>
			SHandle operator()[[noreturn]](const T&) const { Assert(Trap, false) throw 0; }
		};
		/*! LHandle -> WHandle
			SHandle -> WHandle
			WHandle -> WHandle */
		struct GetWHVisitor : boost::static_visitor<WHandle> {
			WHandle operator()(const LHandle& l) const { return l.weak(); }
			WHandle operator()(SHandle s) const { return s.weak(); }
			WHandle operator()(WHandle w) const { return w; }
			template <class T>
			WHandle operator()[[noreturn]](const T&) const { Assert(Trap, false) throw 0; }
		};
	}
	SHandle LCValue::_toHandle(SHandle*) const {
		return boost::apply_visitor(GetSHVisitor(), *this);
	}
	WHandle LCValue::_toHandle(WHandle*) const {
		return boost::apply_visitor(GetWHVisitor(), *this);
	}

	LCValue::LCValue(): LCVar(boost::blank()) {}
	LCValue::LCValue(const LCValue& lc): LCVar(static_cast<const LCVar&>(lc)) {}
	LCValue::LCValue(LCValue&& lcv): LCVar(std::move(static_cast<LCVar&>(lcv))) {}
	LCValue::LCValue(lua_OtherNumber num): LCValue(static_cast<lua_Number>(num)) {}
	LCValue::LCValue(lua_IntegerU num): LCValue(static_cast<lua_Integer>(num)) {}
	LCValue::LCValue(lua_OtherInteger num): LCValue(static_cast<lua_Integer>(num)) {}
	LCValue::LCValue(lua_OtherIntegerU num): LCValue(static_cast<lua_Integer>(num)) {}
	bool LCValue::operator == (const LCValue& lcv) const {
		return static_cast<const LCVar&>(*this) == static_cast<const LCVar&>(lcv);
	}
	bool LCValue::operator != (const LCValue& lcv) const {
		return !(this->operator == (lcv));
	}
	namespace {
		struct ConvertBool : boost::static_visitor<bool> {
			bool operator()(boost::blank) const { return false; }
			bool operator()(LuaNil) const { return false; }
			bool operator()(bool b) const { return b; }
			template <class T>
			bool operator()(const T&) const { return true; }
		};
		const LCValue c_dummyLCValue;
		struct ReferenceIndex : boost::static_visitor<const LCValue&> {
			const int index;
			ReferenceIndex(int idx): index(idx) {}
			const LCValue& operator()(const SPLCTable& sp) const {
				auto itr = sp->find(index);
				if(itr != sp->end())
					return itr->second;
				return c_dummyLCValue;
			}
			template <class T>
			const LCValue& operator()(const T&) const {
				Assert(Trap, false, "invalid LCValue type")
				return c_dummyLCValue;
			}
		};
	}
	LCValue::operator bool () const {
		return boost::apply_visitor(ConvertBool(), *this);
	}
	const LCValue& LCValue::operator [](int s) const {
		// Luaの配列インデックスは1オリジンのため、1を加える
		return boost::apply_visitor(ReferenceIndex(s+1), *this);
	}
	LCValue::LCValue(std::tuple<>&): LCValue() {}
	LCValue::LCValue(std::tuple<>&&): LCValue() {}
	LCValue::LCValue(const std::tuple<>&): LCValue() {}

	LCValue& LCValue::operator = (const LCValue& lcv) {
		this->~LCValue();
		new(this) LCValue(static_cast<const LCVar&>(lcv));
		return *this;
	}
	LCValue& LCValue::operator = (LCValue&& lcv) {
		this->~LCValue();
		new(this) LCValue(std::move(lcv));
		return *this;
	}
	void LCValue::push(lua_State* ls) const {
		Visitor visitor(ls);
		boost::apply_visitor(visitor, *this);
	}
	LuaType LCValue::type() const {
		return boost::apply_visitor(TypeVisitor(), *this);
	}
	std::ostream& LCValue::print(std::ostream& os) const {
		return *boost::apply_visitor(PrintVisitor(os), *this);
	}
	namespace {
		struct LCVisitor : boost::static_visitor<const char*> {
			template <class T>
			const char* operator()(T) const {
				return nullptr;
			}
			const char* operator()(const char* c) const { return c; }
			const char* operator()(const std::string& s) const { return s.c_str(); }
		};
	}
	const char* LCValue::toCStr() const {
		return boost::apply_visitor(LCVisitor(), *this);
	}
	std::string LCValue::toString() const {
		auto* str = toCStr();
		if(str)
			return std::string(str);
		return std::string();
	}
	std::ostream& operator << (std::ostream& os, const LCValue& lcv) {
		return lcv.print(os);
	}
	// ------------------- LV_Global -------------------
	const std::string LV_Global::cs_entry("CS_ENTRY");
	spn::FreeList<int> LV_Global::s_index(std::numeric_limits<int>::max(), 1);
	LV_Global::LV_Global(lua_State* ls) {
		_init(LuaState::GetMainLS_SP(ls));
	}
	LV_Global::LV_Global(const SPLua& sp, const LCValue& lcv) {
		lcv.push(sp->getLS());
		_init(sp);
	}
	LV_Global::LV_Global(const SPLua& sp) {
		_init(sp);
	}
	LV_Global::LV_Global(const LV_Global& lv) {
		lv._prepareValue(true);
		_init(lv._lua);
	}
	LV_Global::LV_Global(LV_Global&& lv):
		_lua(std::move(lv._lua)),
		_id(lv._id)
	{}
	LV_Global::~LV_Global() {
		if(_lua) {
			// エントリの削除
			_lua->getGlobal(cs_entry);
			_lua->setField(-1, _id, LuaNil());
			_lua->pop(1);
			s_index.put(_id);
		}
	}
	void LV_Global::_init(const SPLua& sp) {
		_lua = sp;
		_id = s_index.get();
		_setValue();
	}
	void LV_Global::_setValue() {
		// エントリの登録
		_lua->getGlobal(cs_entry);
		if(_lua->type(-1) != LuaType::Table) {
			_lua->pop(1);
			_lua->newTable();
			_lua->pushValue(-1);
			_lua->setGlobal(cs_entry);
		}
		_lua->push(_id);
		_lua->pushValue(-3);
		// [Value][Entry][id][Value]
		_lua->setTable(-3);
		_lua->pop(2);
	}

	int LV_Global::_prepareValue(bool /*bTop*/) const {
		_lua->getGlobal(cs_entry);
		_lua->getField(-1, _id);
		// [Entry][Value]
		_lua->remove(-2);
		return _lua->getTop();
	}
	void LV_Global::_prepareValue(lua_State* ls) const {
		VPop vp(*this, true);
		lua_State* mls = _lua->getLS();
		lua_xmove(mls, ls, 1);
	}
	void LV_Global::_cleanValue(int pos) const {
		_lua->remove(pos);
	}
	lua_State* LV_Global::getLS() const {
		return _lua->getLS();
	}
	void LV_Global::swap(LV_Global& lv) noexcept {
		std::swap(_lua, lv._lua);
		std::swap(_id, lv._id);
	}
	std::ostream& operator << (std::ostream& os, const LV_Global& v) {
		typename LV_Global::VPop vp(v, true);
		return os << *v._lua;
	}
	std::ostream& operator << (std::ostream& os, const LV_Stack& v) {
		typename LV_Stack::VPop vp(v, true);
		return os << LCValue(v._ls);
	}

	// ------------------- LV_Stack -------------------
	LV_Stack::LV_Stack(lua_State* ls) {
		_init(ls);
	}
	LV_Stack::LV_Stack(lua_State* ls, const LCValue& lcv) {
		lcv.push(ls);
		_init(ls);
	}
	LV_Stack::~LV_Stack() {
		if(!std::uncaught_exception()) {
			AssertP(Trap, lua_gettop(_ls) >= _pos)
			#ifdef DEBUG
				AssertP(Trap, LuaState::SType(_ls, _pos) == _type)
			#endif
			lua_remove(_ls, _pos);
		}
	}
	void LV_Stack::_init(lua_State* ls) {
		_ls = ls;
		_pos = lua_gettop(ls);
		Assert(Trap, _pos > 0)
		#ifdef DEBUG
			_type = LuaState::SType(ls, _pos);
		#endif
	}
	void LV_Stack::_setValue() {
		lua_replace(_ls, _pos);
		#ifdef DEBUG
			_type = LuaState::SType(_ls, _pos);
		#endif
	}
	LV_Stack& LV_Stack::operator = (const LCValue& lcv) {
		lcv.push(_ls);
		_setValue();
		return *this;
	}
	LV_Stack& LV_Stack::operator = (lua_State* ls) {
		if(ls != _ls) {
			lua_pushthread(ls);
			lua_xmove(ls, _ls, 1);
		} else
			lua_pushthread(_ls);
		_setValue();
		return *this;
	}

	int LV_Stack::_prepareValue(bool bTop) const {
		if(bTop) {
			lua_pushvalue(_ls, _pos);
			return lua_gettop(_ls);
		}
		return _pos;
	}
	void LV_Stack::_prepareValue(lua_State* ls) const {
		_prepareValue(true);
		lua_xmove(_ls, ls, 1);
	}
	void LV_Stack::_cleanValue(int pos) const {
		if(_pos < pos)
			lua_remove(_ls, pos);
	}
	lua_State* LV_Stack::getLS() const {
		return _ls;
	}
}
