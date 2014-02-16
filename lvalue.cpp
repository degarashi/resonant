#include "luaw.hpp"
#include <sstream>
#include <limits>

// void LCTable::pushValue(LuaState& ls) const {
// 	ls.newTable();
// 	for(auto& ent : *this)
// 		ls.setField(-1, *ent.first, *ent.second);
// }
// LValue::LValue(const SPLua& spLua, int idx):_spLua(spLua) {
// }
// namespace {
// 	struct Printer : boost::static_visitor<> {
// 		static std::stringstream ss;
//
// 		Printer() {
// 			ss.str("");
// 			ss.clear();
// 		}
// 		void operator()(boost::blank) {
// 			ss << "(none)"; }
// 		template <class T>
// 		void operator()(const T& t) const {
// 			ss << t; }
// 		void operator()(const SPLua& sp) const {
// 			ss << "(LuaState: " << std::hex << sp.get() << std::dec << ")"; }
// 		void operator()(const void* p) const {
// 			ss << "(Ptr: " << std::hex << p << std::dec << ")"; }
// 	};
// 	std::stringstream Printer::ss;
// }
// std::ostream& operator << (std::ostream& os, const LCValue& lcv) {
// 	return os;
// }
// std::ostream& operator << (std::ostream& os, const LCTable& lct) {
// 	return os;
// }

// ------------------- LCValue -------------------
void LCV<boost::blank>::operator()(lua_State* ls, boost::blank t) const {
	lua_pushnil(ls); }
std::ostream& LCV<boost::blank>::operator()(std::ostream& os, boost::blank) const {
	return os << "(none)"; }
LuaType LCV<boost::blank>::operator()() const {
	return LuaType::None; }

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
// 	LuaState lso(_ls);
// 	tbl.pushValue(lso);
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
