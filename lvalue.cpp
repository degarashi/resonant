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
	// ------------------- LCValue -------------------
	// --- LCV<boost::blank> = LUA_TNONE
	void LCV<boost::blank>::operator()(lua_State* /*ls*/, boost::blank) const {
		Assert(Trap, false); }
	boost::blank LCV<boost::blank>::operator()(int /*idx*/, lua_State* /*ls*/, LPointerSP* /*spm*/) const {
		Assert(Trap, false); throw 0; }
	std::ostream& LCV<boost::blank>::operator()(std::ostream& os, boost::blank) const {
		return os << "(none)"; }
	LuaType LCV<boost::blank>::operator()() const {
		return LuaType::LNone; }

	// --- LCV<LuaNil> = LUA_TNIL
	void LCV<LuaNil>::operator()(lua_State* ls, LuaNil) const {
		lua_pushnil(ls); }
	LuaNil LCV<LuaNil>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Nil);
		return LuaNil(); }
	std::ostream& LCV<LuaNil>::operator()(std::ostream& os, LuaNil) const {
		return os << "(nil)"; }
	LuaType LCV<LuaNil>::operator()() const {
		return LuaType::Nil; }

	// --- LCV<bool> = LUA_TBOOL
	void LCV<bool>::operator()(lua_State* ls, bool b) const {
		lua_pushboolean(ls, b); }
	bool LCV<bool>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Boolean);
		return lua_toboolean(ls, idx) != 0; }
	std::ostream& LCV<bool>::operator()(std::ostream& os, bool b) const {
		return os << std::boolalpha << b; }
	LuaType LCV<bool>::operator()() const {
		return LuaType::Boolean; }

	// --- LCV<const char*> = LUA_TSTRING
	void LCV<const char*>::operator()(lua_State* ls, const char* c) const {
		lua_pushstring(ls, c); }
	const char* LCV<const char*>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		return lua_tostring(ls, idx); }
	std::ostream& LCV<const char*>::operator()(std::ostream& os, const char* c) const {
		return os << c; }
	LuaType LCV<const char*>::operator()() const {
		return LuaType::String; }

	// --- LCV<std::string> = LUA_TSTRING
	void LCV<std::string>::operator()(lua_State* ls, const std::string& s) const {
		lua_pushlstring(ls, s.c_str(), s.length()); }
	std::string LCV<std::string>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		size_t len;
		const char* c =lua_tolstring(ls, idx, &len);
		return std::string(c, len); }
	std::ostream& LCV<std::string>::operator()(std::ostream& os, const std::string& s) const {
		return os << s; }
	LuaType LCV<std::string>::operator()() const {
		return LuaType::String; }

	// --- LCV<lua_Integer> = LUA_TNUMBER
	void LCV<lua_Integer>::operator()(lua_State* ls, lua_Integer i) const {
		lua_pushinteger(ls, i); }
	lua_Integer LCV<lua_Integer>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tointeger(ls, idx); }
	std::ostream& LCV<lua_Integer>::operator()(std::ostream& os, lua_Integer i) const {
		return os << i; }
	LuaType LCV<lua_Integer>::operator()() const {
		return LuaType::Number; }

	// --- LCV<lua_Number> = LUA_TNUMBER
	void LCV<lua_Number>::operator()(lua_State* ls, lua_Number f) const {
		lua_pushnumber(ls, f); }
	lua_Number LCV<lua_Number>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tonumber(ls, idx); }
	std::ostream& LCV<lua_Number>::operator()(std::ostream& os, lua_Number f) const {
		return os << f; }
	LuaType LCV<lua_Number>::operator()() const {
		return LuaType::Number; }

	// --- LCV<lua_State*> = LUA_TTHREAD
	void LCV<lua_State*>::operator()(lua_State* ls, lua_State* lsp) const {
		if(ls == lsp)
			lua_pushthread(ls);
		else {
			lua_pushthread(lsp);
			lua_xmove(lsp, ls, 1);
		}
	}
	lua_State* LCV<lua_State*>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		return lua_tothread(ls, idx);
	}
	std::ostream& LCV<lua_State*>::operator()(std::ostream& os, lua_State* ls) const {
		return os << "(thread) " << (uintptr_t)ls;
	}
	LuaType LCV<lua_State*>::operator()() const {
		return LuaType::Thread; }

	// --- LCV<spn::Size> = LUA_TTABLE
	void LCV<spn::SizeF>::operator()(lua_State* ls, const spn::SizeF& s) const {
		lua_createtable(ls, 2, 0);
		lua_pushinteger(ls, 1);
		lua_pushnumber(ls, s.width);
		lua_settable(ls, -3);
		lua_pushinteger(ls, 2);
		lua_pushnumber(ls, s.height);
		lua_settable(ls, -3);
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

	// --- LCV<SPLua> = LUA_TTHREAD
	void LCV<SPLua>::operator()(lua_State* ls, const SPLua& sp) const {
		sp->pushSelf();
		lua_xmove(sp->getLS(), ls, 1);
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

	// --- LCV<void*> = LUA_TLIGHTUSERDATA
	void LCV<void*>::operator()(lua_State* ls, const void* ud) const {
		lua_pushlightuserdata(ls, const_cast<void*>(ud)); }
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

	// --- LCV<lua_CFunction> = LUA_TFUNCTION
	void LCV<lua_CFunction>::operator()(lua_State* ls, lua_CFunction f) const {
		lua_pushcclosure(ls, f, 0); }
	lua_CFunction LCV<lua_CFunction>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LuaState::_CheckType(ls, idx, LuaType::Function);
		return lua_tocfunction(ls, idx);
	}
	std::ostream& LCV<lua_CFunction>::operator()(std::ostream& os, lua_CFunction f) const {
		return os << "(function)" << std::hex << reinterpret_cast<uintptr_t>(f); }
	LuaType LCV<lua_CFunction>::operator()() const {
		return LuaType::Function; }

	// --- LCV<Timepoint> = LUA_TNUMBER
	void LCV<Timepoint>::operator()(lua_State* ls, const Timepoint& t) const {
		LCV<Duration> dur;
		dur(ls, Duration(t.time_since_epoch()));
	}
	Timepoint LCV<Timepoint>::operator()(int idx, lua_State* ls, LPointerSP* /*spm*/) const {
		LCV<Duration> dur;
		return Timepoint(dur(idx, ls));
	}
	std::ostream& LCV<Timepoint>::operator()(std::ostream& os, const Timepoint& t) const {
		return os << t; }
	LuaType LCV<Timepoint>::operator()() const {
		return LuaType::Number; }

	// --- LCV<LCTable> = LUA_TTABLE
	void LCV<LCTable>::operator()(lua_State* ls, const LCTable& t) const {
		LuaState lsc(ls);
		lsc.newTable(0, t.size());
		for(auto& ent : t) {
			lsc.setField(-1, ent.first, ent.second);
		}
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
	void LCV<LCValue>::operator()(lua_State* ls, const LCValue& lcv) const {
		lcv.push(ls); }
	LCValue LCV<LCValue>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		int typ = lua_type(ls, idx);
		return c_toLCValue[typ+1](ls, idx, spm);
	}
	std::ostream& LCV<LCValue>::operator()(std::ostream& os, const LCValue& lcv) const {
		return os << lcv; }
	LuaType LCV<LCValue>::operator()() const {
		return LuaType::LNone; }

	// --- LCV<LValueG>
	void LCV<LValueG>::operator()(lua_State* ls, const LValueG& t) const {
		t.prepareValue(ls);
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
	LCValue::LCValue(const spn::Vec4& v): LCVar(v) {}
	LCValue& LCValue::operator = (const spn::Vec4& v) {
		static_cast<LCVar&>(*this) = v;
		return *this;
	}
	bool LCValue::operator == (const LCValue& lcv) const {
		return static_cast<const LCVar&>(*this) == static_cast<const LCVar&>(lcv);
	}
	bool LCValue::operator != (const LCValue& lcv) const {
		return !(this->operator == (lcv));
	}

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
		lv._prepareValue();
		_init(lv._lua);
	}
	LV_Global::LV_Global(LV_Global&& lv): _lua(std::move(lv._lua)), _id(lv._id) {}
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

	int LV_Global::_prepareValue() const {
		_lua->getGlobal(cs_entry);
		_lua->getField(-1, _id);
		// [Entry][Value]
		_lua->remove(-2);
		return _lua->getTop();
	}
	int LV_Global::_prepareValue(lua_State* ls) const {
		_prepareValue();
		lua_State* mls = _lua->getLS();
		if(mls != ls)
			lua_xmove(mls, ls, 1);
		return lua_gettop(ls);
	}
	void LV_Global::_cleanValue() const {
		_lua->pop(1);
	}
	void LV_Global::_cleanValue(lua_State* ls) const {
		lua_pop(ls, 1);
	}
	lua_State* LV_Global::getLS() const {
		return _lua->getLS();
	}
	std::ostream& operator << (std::ostream& os, const LV_Global& v) {
		v._prepareValue();
		os << *v._lua;
		v._cleanValue();
		return os;
	}
	std::ostream& operator << (std::ostream& os, const LV_Stack& v) {
		v._prepareValue();
		LCValue lcv(v._ls);
		return os << lcv;
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
		_pos = lua_gettop(_ls);
		Assert(Trap, _pos > 0)
		#ifdef DEBUG
			_type = LuaState::SType(ls, _pos);
		#endif
	}
	void LV_Stack::_setValue() {
		lua_replace(_ls, _pos);
	}
	LV_Stack& LV_Stack::operator = (const LCValue& lcv) {
		lcv.push(_ls);
		lua_replace(_ls, _pos);
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

	int LV_Stack::_prepareValue() const {
		lua_pushvalue(_ls, _pos);
		return lua_gettop(_ls);
	}
	int LV_Stack::_prepareValue(lua_State* ls) const {
		lua_pushvalue(_ls, _pos);
		if(_ls != ls)
			lua_xmove(_ls, ls, 1);
		return lua_gettop(ls);
	}
	void LV_Stack::_cleanValue() const {
		_cleanValue(_ls);
	}
	void LV_Stack::_cleanValue(lua_State* ls) const {
		lua_pop(ls, 1);
	}
	lua_State* LV_Stack::getLS() const {
		return _ls;
	}
}

