#include "luaw.hpp"
#include <fstream>
#include <sstream>
#include <limits>

// ----------------- LuaState::Exceptions -----------------
LuaState::EBase::EBase(const std::string& typ_msg, const std::string& msg): std::runtime_error("") {
	std::stringstream ss;
	ss << "Error in LuaState:\n" << typ_msg << std::endl << msg;
	reinterpret_cast<std::runtime_error&>(*this) = std::runtime_error(ss.str());
}
LuaState::ERun::ERun(const std::string& s): EBase("Runtime", s) {}
LuaState::ESyntax::ESyntax(const std::string& s): EBase("Syntax", s) {}
LuaState::EMem::EMem(const std::string& s): EBase("Memory", s) {}
LuaState::EError::EError(const std::string& s): EBase("ErrorHandler", s) {}
LuaState::EGC::EGC(const std::string& s): EBase("GC", s) {}
LuaState::EType::EType(const char* typ0, const char* typ1): EBase("InvalidType", std::string(typ0) + " to " + typ1) {}

// ----------------- LuaState -----------------
const std::string LuaState::cs_fromId("FromId"),
					LuaState::cs_fromThread("FromThread"),
					LuaState::cs_mainThread("MainThread");
const int LuaState::ENT_ID = 1,
		  LuaState::ENT_THREAD = 2,
		  LuaState::ENT_NREF = 3;
spn::FreeList<int> LuaState::s_index(std::numeric_limits<int>::max(), 1);
void LuaState::Nothing(lua_State* ls) {}
void LuaState::Delete(lua_State* ls) {
	lua_close(ls);
}
LuaState::LuaState(const SPLua& spLua) {
	int id = s_index.get();
	_id = id;

	int top = spLua->getTop();
	spLua->getGlobal(cs_fromId);
	spLua->getGlobal(cs_fromThread);
	if(!spLua->isTable(-1)) {
		spLua->pop(2);

		spLua->newTable();
		spLua->pushValue(-1);
		spLua->setGlobal(cs_fromId);

		spLua->newTable();
		spLua->pushValue(-1);
		spLua->setGlobal(cs_fromThread);
	}
	auto fnDel = [id](lua_State* ls) {
		LuaState lsc(ls);
		int top = lsc.getTop();
		lsc.getGlobal(cs_fromId);
		lsc.getField(-1, id);
		// [fromId][123]
		lsc.getField(-1, int(ENT_NREF));
		// [fromId][123][nRef]
		int nRef = lsc.toInteger(-1);
		if(--nRef == 0) {
			lsc.pop(1);
			// [fromId][123]
			lsc.setField(-2, id, boost::blank());
			lsc.getGlobal(cs_fromThread);
			// [fromId][123][fromThread]
			lsc.getField(-2, ENT_THREAD);
			lsc.push(boost::blank());
			// [fromId][123][fromThread][Thread][nil]
			lsc.setTable(-3);
		} else
			lsc.setField(-2, id, nRef);
		lsc.setTop(top);
	};
	// G[id] = {1=Id, 2=Thread, 3=NRef}
	spLua->newTable();
	spLua->setField(-1, ENT_ID, id);
	spLua->push(ENT_THREAD);
	lua_State* nls = lua_newthread(spLua->getLS());
	_lua = SPILua(nls, fnDel);
	_base = spLua->getMainLS();
	spLua->setTable(-3);
	spLua->setField(-1, ENT_NREF, 1);

	// [fromId][fromThread][Thread][123]
	// FromId
	spLua->push(id);
	spLua->pushValue(-2);
	// [fromId][fromThread][Thread][123][id][123]
	spLua->setTable(-6);

	// FromThread
	spLua->pushValue(-2);
	spLua->pushValue(-2);
	// [fromId][fromThread][Thread][123][Thread][123]
	spLua->setTable(-5);

	spLua->setTop(top);
}
LuaState::LuaState(lua_State* ls, _TagThread) {
	LuaState lsc(ls);
	int top = lsc.getTop();

	// 参照カウンタのインクリメント
	lsc.getGlobal(cs_fromThread);
	lsc.pushSelf();
	lsc.getTable(-1);
	// [fromThread][123]
	lsc.getField(-1, ENT_NREF);
	int nRef = lsc.toInteger(-1);
	lsc.pop(1);
	lsc.setField(-1, ENT_NREF, ++nRef);

	lsc.getGlobal(cs_mainThread);
	void* pMain = lsc.toUserData(-1);
	_base = reinterpret_cast<LuaState*>(pMain)->shared_from_this();

	lsc.setTop(top);
}

LuaState::LuaState(const SPILua& ls):
	_lua(ls.get(), Nothing)  {}
LuaState::LuaState(lua_State* ls):
	_lua(ls, Nothing) {}
LuaState::LuaState(lua_Alloc f, void* ud) {
	_lua = SPILua(f ? lua_newstate(f, ud) : luaL_newstate(), Delete);
	push(static_cast<void*>(this));
	setGlobal(cs_mainThread);
}
LuaState::LuaState(LuaState&& ls): _base(std::move(ls._base)), _lua(std::move(ls._lua)) {}
void LuaState::load(const std::string& fname) {
	std::ifstream ifs(fname, std::ios::binary|std::ios::in);
	if(!ifs.is_open())
		throw std::runtime_error("file not found");
	ifs.seekg(0, std::ios::end);
	int pos = ifs.tellg();
	ifs.seekg(0, std::ios::beg);

	// メモリに一括読み込み
	std::vector<char> buff(pos + 1);
	ifs.read(buff.data(), pos);
	ifs.close();
	buff.back() = '\0';

	int res = luaL_loadstring(getLS(), buff.data());
	_checkError(res);
	// Loadされたチャンクを実行
	call(0,0);
}
void LuaState::pushSelf() {
	lua_pushthread(getLS());
}
void LuaState::loadLibraries() {
	luaL_openlibs(getLS());
}

namespace {
	struct Visitor : boost::static_visitor<> {
		lua_State* _ls;
		Visitor(lua_State* ls): _ls(ls) {}
		void operator()(boost::blank b) const { lua_pushnil(_ls); }
		void operator()(bool b) const { lua_pushboolean(_ls, b); }
		void operator()(const char* c) const { lua_pushstring(_ls, c); }
		void operator()(lua_Integer i) const { lua_pushinteger(_ls, i); }
		void operator()(lua_Unsigned i) const { lua_pushinteger(_ls, i); }
		void operator()(float f) const { lua_pushnumber(_ls, f); }
		void operator()(double d) const { lua_pushnumber(_ls, d); }
		void operator()(const SPLua& ls) {}
		void operator()(void* ud) { lua_pushlightuserdata(_ls, ud); }
		void operator()(lua_CFunction f) { lua_pushcclosure(_ls, f, 0); }
		void operator()(const std::string& s) const { lua_pushlstring(_ls, s.c_str(), s.length()); }
		void operator()(const LCTable& tbl) const {
// 			LuaState lso(_ls);
// 			tbl.pushValue(lso);
		}
	};
}
void LuaState::push(const LCValue& v) {
	Visitor visitor(getLS());
	boost::apply_visitor(visitor, v);
}
void LuaState::pushValue(int idx) {
	lua_pushvalue(getLS(), idx);
}
void LuaState::pop(int n) {
	lua_pop(getLS(), n);
}
int LuaState::absIndex(int idx) const {
	return lua_absindex(getLS(), idx);
}
void LuaState::arith(OP op) {
	lua_arith(getLS(), static_cast<int>(op));
}
lua_CFunction LuaState::atPanic(lua_CFunction panicf) {
	return lua_atpanic(getLS(), panicf);
}
void LuaState::call(int nargs, int nresults) {
	int err = lua_pcall(getLS(), nargs, nresults, 0);
	_checkError(err);
}
void LuaState::callk(int nargs, int nresults, int ctx, lua_CFunction k) {
	int err = lua_pcallk(getLS(), nargs, nresults, 0, ctx, k);
	_checkError(err);
}
bool LuaState::checkStack(int extra) {
	return lua_checkstack(getLS(), extra) != 0;
}
bool LuaState::compare(int idx0, int idx1, CMP cmp) const {
	return lua_compare(getLS(), idx0, idx1, static_cast<int>(cmp)) != 0;
}
void LuaState::concat(int n) {
	lua_concat(getLS(), n);
}
void LuaState::copy(int from, int to) {
	lua_copy(getLS(), from, to);
}
void LuaState::dump(lua_Writer writer, void* data) {
	lua_dump(getLS(), writer, data);
}
void LuaState::error() {
	lua_error(getLS());
}
int LuaState::gc(GC what, int data) {
	return lua_gc(getLS(), static_cast<int>(what), data);
}
lua_Alloc LuaState::getAllocf(void** ud) const {
	return lua_getallocf(getLS(), ud);
}
int LuaState::getCtx(int* ctx) const {
	return lua_getctx(getLS(), ctx);
}
void LuaState::getField(int idx, const LCValue& key) {
	idx = absIndex(idx);
	push(key);
	getTable(idx);
}
void LuaState::getGlobal(const LCValue& key) {
	pushGlobal();
	push(key);
	// [Global][key]
	getTable(-2);
	remove(-2);
}
void LuaState::getTable(int idx) {
	lua_gettable(getLS(), idx);
}
int LuaState::getTop() const {
	return lua_gettop(getLS());
}
void LuaState::getUserValue(int idx) {
	lua_getuservalue(getLS(), idx);
}
void LuaState::insert(int idx) {
	lua_insert(getLS(), idx);
}
bool LuaState::isBoolean(int idx) const {
	return lua_isboolean(getLS(), idx) != 0;
}
bool LuaState::isCFunction(int idx) const {
	return lua_iscfunction(getLS(), idx) != 0;
}
bool LuaState::isLightUserdata(int idx) const {
	return lua_islightuserdata(getLS(), idx) != 0;
}
bool LuaState::isNil(int idx) const {
	return lua_isnil(getLS(), idx) != 0;
}
bool LuaState::isNone(int idx) const {
	return lua_isnone(getLS(), idx) != 0;
}
bool LuaState::isNoneOrNil(int idx) const {
	return lua_isnoneornil(getLS(), idx) != 0;
}
bool LuaState::isNumber(int idx) const {
	return lua_isnumber(getLS(), idx) != 0;
}
bool LuaState::isString(int idx) const {
	return lua_isstring(getLS(), idx) != 0;
}
bool LuaState::isTable(int idx) const {
	return lua_istable(getLS(), idx) != 0;
}
bool LuaState::isThread(int idx) const {
	return lua_isthread(getLS(), idx) != 0;
}
bool LuaState::isUserdata(int idx) const {
	return lua_isuserdata(getLS(), idx) != 0;
}
void LuaState::length(int idx) {
	lua_len(getLS(), idx);
}
void LuaState::newTable(int narr, int nrec) {
	lua_createtable(getLS(), narr, nrec);
}
LuaState LuaState::newThread() {
	return LuaState(shared_from_this());
}
void* LuaState::newUserData(size_t sz) {
	return lua_newuserdata(getLS(), sz);
}
int LuaState::next(int idx) {
	return lua_next(getLS(), idx);
}
bool LuaState::rawEqual(int idx0, int idx1) {
	return lua_rawequal(getLS(), idx0, idx1) != 0;
}
void LuaState::rawGet(int idx) {
	lua_rawget(getLS(), idx);
}
void LuaState::rawGet(int idx, const LCValue& v) {
	push(v);
	rawGet(idx);
}
size_t LuaState::rawLen(int idx) const {
	return lua_rawlen(getLS(), idx);
}
void LuaState::rawSet(int idx) {
	lua_rawset(getLS(), idx);
}
void LuaState::rawSet(int idx, const LCValue& v) {
	push(v);
	rawSet(idx);
}
void LuaState::remove(int idx) {
	lua_remove(getLS(), idx);
}
void LuaState::replace(int idx) {
	lua_replace(getLS(), idx);
}
bool LuaState::resume(const SPLua& from, int narg) {
	SPLua mls(getMainLS());
	lua_State *ls0 = mls->getLS(),
				*ls1 = getLS();
	if(ls0 == ls1)
		return false;
	int res = lua_resume(ls0, ls1, narg);
	_checkError(res);
	return res == LUA_YIELD;
}
void LuaState::setAllocf(lua_Alloc f, void* ud) {
	lua_setallocf(getLS(), f, ud);
}
void LuaState::setField(int idx, const LCValue& key, const LCValue& val) {
	idx = absIndex(idx);
	push(key);
	push(val);
	setTable(idx);
}
void LuaState::setGlobal(const LCValue& key) {
	pushGlobal();
	push(key);
	pushValue(-3);
	// [value][Global][key][value]
	setTable(-3);
	pop(2);
}
void LuaState::pushGlobal() {
	pushValue(LUA_REGISTRYINDEX);
	getField(-1, LUA_RIDX_GLOBALS);
	// [Registry][Global]
	remove(-2);
}
void LuaState::setTable(int idx) {
	lua_settable(getLS(), idx);
}
void LuaState::setTop(int idx) {
	lua_settop(getLS(), idx);
}
void LuaState::setUservalue(int idx) {
	lua_setuservalue(getLS(), idx);
}
bool LuaState::status() const {
	int res = lua_status(getLS());
	_checkError(res);
	return res != 0;
}
void LuaState::_checkType(int idx, Type typ) const {
	Type t = type(idx);
	if(t != typ)
		throw EType(typeName(t), typeName(typ));
}
bool LuaState::toBoolean(int idx) const {
	_checkType(idx, Type::Boolean);
	return lua_toboolean(getLS(), idx) != 0;
}
lua_CFunction LuaState::toCFunction(int idx) const {
	_checkType(idx, Type::Function);
	return lua_tocfunction(getLS(), idx);
}
lua_Integer LuaState::toInteger(int idx) const {
	_checkType(idx, Type::Number);
	return lua_tointeger(getLS(), idx);
}
std::pair<const char*, size_t> LuaState::toString(int idx) const {
	_checkType(idx, Type::String);
	size_t len;
	const char* str = lua_tolstring(getLS(), idx, &len);
	return std::make_pair(str, len);
}
lua_Number LuaState::toNumber(int idx) const {
	_checkType(idx, Type::Number);
	return lua_tonumber(getLS(), idx);
}
const void* LuaState::toPointer(int idx) const {
	return lua_topointer(getLS(), idx);
}
SPLua LuaState::toThread(int idx) const {
	_checkType(idx, Type::Thread);
	return SPLua(new LuaState(lua_tothread(getLS(), idx)));
}
lua_Unsigned LuaState::toUnsigned(int idx) const {
	_checkType(idx, Type::Number);
	return lua_tounsignedx(getLS(), idx, nullptr);
}
void* LuaState::toUserData(int idx) const {
	try {
		_checkType(idx, Type::Userdata);
	} catch(const EType& e) {
		_checkType(idx, Type::LightUserdata);
	}
	return lua_touserdata(getLS(), idx);
}

LuaState::Type LuaState::type(int idx) const {
	int typ = lua_type(getLS(), idx);
	switch(typ) {
		case LUA_TNIL: return Type::Nil;
		case LUA_TNUMBER: return Type::Number;
		case LUA_TBOOLEAN: return Type::Boolean;
		case LUA_TSTRING: return Type::String;
		case LUA_TTABLE: return Type::Table;
		case LUA_TFUNCTION: return Type::Function;
		case LUA_TUSERDATA: return Type::Userdata;
		case LUA_TTHREAD: return Type::Thread;
		case LUA_TLIGHTUSERDATA: return Type::LightUserdata;
	}
	return Type::None;
}
const char* LuaState::typeName(Type typ) const {
	return lua_typename(getLS(), static_cast<int>(typ));
}
const lua_Number* LuaState::version() const {
	return lua_version(getLS());
}
void LuaState::xmove(const SPLua& to, int n) {
	lua_xmove(getLS(), to->getLS(), n);
}
int LuaState::yield(int nresults) {
	return lua_yield(getLS(), nresults);
}
int LuaState::yieldk(int nresults, int ctx, lua_CFunction k) {
	return lua_yieldk(getLS(), nresults, ctx, k);
}
lua_State* LuaState::getLS() const {
	return _lua.get();
}
SPLua LuaState::getMainLS() {
	if(_base)
		return _base->getMainLS();
	return shared_from_this();
}
void LuaState::_checkError(int code) const {
	if(code != LUA_OK) {
		const char* msg = toString(-1).first;
		switch(code) {
			case LUA_ERRRUN:
				throw ERun(msg);
			case LUA_ERRMEM:
				throw EMem(msg);
			case LUA_ERRERR:
				throw EError(msg);
			case LUA_ERRSYNTAX:
				throw ESyntax(msg);
			case LUA_ERRGCMM:
				throw EGC(msg);
		}
		throw EBase("unknown error-code", msg);
	}
}
std::ostream& operator << (std::ostream& os, const LuaState& ls) {
	// スタックの値を表示する
	int n = ls.getTop();
	for(int i=1 ; i<=n ; i++) {

	}
	return os;
}

