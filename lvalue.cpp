#include "luaw.hpp"
#include <sstream>
#include <limits>

// ------------------- LCValue -------------------
void LCV<boost::blank>::operator()(lua_State* ls, boost::blank) const {
	assert(false); }
std::ostream& LCV<boost::blank>::operator()(std::ostream& os, boost::blank) const {
	return os << "(none)"; }
LuaType LCV<boost::blank>::operator()() const {
	return LuaType::None; }

void LCV<LuaNil>::operator()(lua_State* ls, LuaNil) const {
	lua_pushnil(ls); }
std::ostream& LCV<LuaNil>::operator()(std::ostream& os, LuaNil) const {
	return os << "(nil)"; }
LuaType LCV<LuaNil>::operator()() const {
	return LuaType::Nil; }

void LCV<bool>::operator()(lua_State* ls, bool b) const {
	lua_pushboolean(ls, b); }
std::ostream& LCV<bool>::operator()(std::ostream& os, bool b) const {
	return os << std::boolalpha << b; }
LuaType LCV<bool>::operator()() const {
	return LuaType::Boolean; }

void LCV<const char*>::operator()(lua_State* ls, const char* c) const {
	lua_pushstring(ls, c); }
std::ostream& LCV<const char*>::operator()(std::ostream& os, const char* c) const {
	return os << c; }
LuaType LCV<const char*>::operator()() const {
	return LuaType::String; }

void LCV<std::string>::operator()(lua_State* ls, const std::string& s) const {
	lua_pushlstring(ls, s.c_str(), s.length()); }
std::ostream& LCV<std::string>::operator()(std::ostream& os, const std::string& s) const {
	return os << s; }
LuaType LCV<std::string>::operator()() const {
	return LuaType::String; }

void LCV<lua_Integer>::operator()(lua_State* ls, lua_Integer i) const {
	lua_pushinteger(ls, i); }
std::ostream& LCV<lua_Integer>::operator()(std::ostream& os, lua_Integer i) const {
	return os << i; }
LuaType LCV<lua_Integer>::operator()() const {
	return LuaType::Number; }

void LCV<lua_Unsigned>::operator()(lua_State* ls, lua_Unsigned i) const {
	lua_pushunsigned(ls, i); }
std::ostream& LCV<lua_Unsigned>::operator()(std::ostream& os, lua_Unsigned i) const {
	return os << i; }
LuaType LCV<lua_Unsigned>::operator()() const {
	return LuaType::Number; }

void LCV<lua_Number>::operator()(lua_State* ls, lua_Number f) const {
	lua_pushnumber(ls, f); }
std::ostream& LCV<lua_Number>::operator()(std::ostream& os, lua_Number f) const {
	return os << f; }
LuaType LCV<lua_Number>::operator()() const {
	return LuaType::Number; }

void LCV<SPLua>::operator()(lua_State* ls, const SPLua& sp) const {
}
std::ostream& LCV<SPLua>::operator()(std::ostream& os, const SPLua& sp) const {
	return os;
}
LuaType LCV<SPLua>::operator()() const {
	return LuaType::Thread; }

void LCV<void*>::operator()(lua_State* ls, const void* ud) const {
	lua_pushlightuserdata(ls, const_cast<void*>(ud)); }
std::ostream& LCV<void*>::operator()(std::ostream& os, const void* ud) const {
	return os << "(userdata)" << std::hex << ud; }
LuaType LCV<void*>::operator()() const {
	return LuaType::LightUserdata; }

void LCV<lua_CFunction>::operator()(lua_State* ls, lua_CFunction f) const {
	lua_pushcclosure(ls, f, 0); }
std::ostream& LCV<lua_CFunction>::operator()(std::ostream& os, lua_CFunction f) const {
	return os << "(function)" << std::hex << reinterpret_cast<uintptr_t>(f); }
LuaType LCV<lua_CFunction>::operator()() const {
	return LuaType::Function; }

void LCV<LCTable>::operator()(lua_State* ls, const LCTable& t) const {
	LuaState lsc(ls);
	lsc.newTable(0, t.size());
	for(auto& ent : t) {
		lsc.setField(-1, *ent.first, *ent.second);
	}
}
std::ostream& LCV<LCTable>::operator()(std::ostream& os, const LCTable& t) const {
	return os << "(table)" << std::hex << reinterpret_cast<uintptr_t>(&t); }
LuaType LCV<LCTable>::operator()() const {
	return LuaType::Table; }

namespace {
	struct Visitor : boost::static_visitor<> {
		lua_State* _ls;
		Visitor(lua_State* ls): _ls(ls) {}
		template <class T>
		void operator()(const T& t) const {
			LCV<T>()(_ls, t);
		}
	};
	struct TypeVisitor : boost::static_visitor<LuaType> {
		template <class T>
		LuaType operator()(const T&) const {
			return LCV<T>()(); }
	};
	struct PrintVisitor : boost::static_visitor<std::ostream&> {
		std::ostream& _os;
		PrintVisitor(std::ostream& os): _os(os) {}
		template <class T>
		std::ostream& operator()(const T& t) const {
			return LCV<T>()(_os, t); }
	};
}
void LCValue::push(lua_State* ls) const {
	Visitor visitor(ls);
	boost::apply_visitor(visitor, *this);
}
LuaType LCValue::type() const {
	return boost::apply_visitor(TypeVisitor(), *this);
}
std::ostream& LCValue::print(std::ostream& os) const {
	return boost::apply_visitor(PrintVisitor(os), *this);
}
std::ostream& operator << (std::ostream& os, const LCValue& lcv) {
	return lcv.print(os);
}
// ------------------- LV_Global -------------------
const std::string LV_Global::cs_entry("CS_ENTRY");
spn::FreeList<int> LV_Global::s_index(std::numeric_limits<int>::max(), 1);
LV_Global::LV_Global(lua_State* ls) {
	// 
	
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
	lua_State* mls = _lua->getLS();
	if(mls != ls) {
		_prepareValue();
		lua_xmove(mls, ls, 1);
	}
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

// ------------------- LV_Stack -------------------
LV_Stack::LV_Stack(lua_State* ls) {
	_init(ls);
}
LV_Stack::LV_Stack(lua_State* ls, const LCValue& lcv) {
	lcv.push(ls);
	_init(ls);
}
LV_Stack::~LV_Stack() {
	assert(lua_gettop(_ls) == _pos);
	lua_pop(_ls, 1);
}
void LV_Stack::_init(lua_State* ls) {
	_ls = ls;
	_pos = lua_gettop(_ls);
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
	return _pos;
}
int LV_Stack::_prepareValue(lua_State* ls) const {
	lua_pushvalue(_ls, _pos);
	if(_ls != ls)
		lua_xmove(_ls, ls, 1);
	return lua_gettop(ls);
}
void LV_Stack::_cleanValue() const {}
void LV_Stack::_cleanValue(lua_State* ls) const {
	lua_pop(ls, 1);
}
lua_State* LV_Stack::getLS() const {
	return _ls;
}

