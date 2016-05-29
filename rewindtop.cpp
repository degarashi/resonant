#include "luaw.hpp"
namespace rs {
	RewindTop::RewindTop(lua_State* ls):
		_ls(ls),
		_base(lua_gettop(ls)),
		_bReset(true)
	{}
	RewindTop::~RewindTop() {
		if(_bReset)
			lua_settop(_ls, _base);
	}
	void RewindTop::setReset(const bool r) {
		_bReset = r;
	}
	int RewindTop::getNStack() const {
		return lua_gettop(_ls) - _base;
	}
}
