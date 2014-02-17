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
	// これが呼ばれる時は依存するスレッドなどが全て削除された時なので直接lua_closeを呼ぶ
	lua_close(ls);
}
LuaState::Deleter LuaState::_MakeDeleter(int id) {
	return [id](lua_State* ls) {
		LuaState lsc(ls);
		_Decrement_ID(lsc, id);
	};
}
void LuaState::_LoadThreadTable(LuaState& lsc) {
	// テーブルが存在しない場合は作成
	lsc.getGlobal(cs_fromId);
	lsc.getGlobal(cs_fromThread);
	if(!lsc.isTable(-1)) {
		lsc.pop(2);

		lsc.newTable();
		lsc.pushValue(-1);
		lsc.setGlobal(cs_fromId);

		lsc.newTable();
		lsc.pushValue(-1);
		lsc.setGlobal(cs_fromThread);
	}
}
void LuaState::_Increment(LuaState& lsc, std::function<void (LuaState&)> cb) {
	int top = lsc.getTop();

	_LoadThreadTable(lsc);
	cb(lsc);
	// [fromId][fromThread][123]
	lsc.getField(-1, ENT_NREF);
	int nRef = lsc.toInteger(-1);
	lsc.setField(-2, ENT_NREF, ++nRef);

	lsc.setTop(top);
}
void LuaState::_Increment_ID(LuaState& lsc, int id) {
	_Increment(lsc, [id](LuaState& lsc){ lsc.getField(-2, id); });
}
void LuaState::_Increment_Th(LuaState& lsc) {
	int pos = lsc.getTop();
	_Increment(lsc, [pos](LuaState& lsc){
		lsc.pushValue(pos);
		lsc.getTable(-2);
	});
	lsc.pop(1);
}
void LuaState::_Decrement_ID(LuaState& lsc, int id) {
	_Decrement(lsc, [id](LuaState& lsc){ lsc.getField(-2, id); });
}
void LuaState::_Decrement_Th(LuaState& lsc) {
	int pos = lsc.getTop();
	_Decrement(lsc, [pos](LuaState& lsc){
		lsc.pushValue(pos);
		lsc.getTable(-2);
	});
	lsc.pop(1);
}
void LuaState::_Decrement(LuaState& lsc, std::function<void (LuaState&)> cb) {
	int top = lsc.getTop();

	_LoadThreadTable(lsc);
	cb(lsc);
	// [fromId][fromThread][123]
	lsc.getField(-1, ENT_NREF);
	int nRef = lsc.toInteger(-1);
	if(--nRef == 0) {
		// エントリを消去
		lsc.getField(-2, ENT_ID);
		lsc.push(LuaNil());
		// [fromId][fromThread][123][nRef][ID][Nil]
		lsc.setTable(-6);
		// [fromId][fromThread][123][nRef]
		lsc.getField(-2, ENT_THREAD);
		lsc.push(LuaNil());
		// [fromId][fromThread][123][nRef][Thread][Nil]
		lsc.setTable(-5);
	} else {
		// デクリメントした値を代入
		lsc.setField(-2, ENT_NREF, nRef);
	}

	lsc.setTop(top);
}
SPILua LuaState::_RegisterNewThread(LuaState& lsc, int id) {
	int top = lsc.getTop();

	_LoadThreadTable(lsc);
	// G[id] = {1=Id, 2=Thread, 3=NRef}
	lsc.newTable();
	lsc.setField(-1, ENT_ID, id);
	lua_State* nls = lua_newthread(lsc.getLS());
	lsc.push(ENT_THREAD);
	lsc.pushValue(-2);
	// [fromId][fromThread][123][Thread][ENT_THREAD][Thread]
	lsc.setTable(-4);
	// [fromId][fromThread][123][Thread]
	lsc.setField(-2, ENT_NREF, 1);

	// FromId
	lsc.push(id);
	lsc.pushValue(-3);
	// [fromId][fromThread][123][Thread][id][123]
	lsc.setTable(-6);

	// FromThread
	lsc.pushValue(-1);
	lsc.pushValue(-3);
	// [fromId][fromThread][123][Thread][Thread][123]
	lsc.setTable(-5);

	lsc.setTop(top);
	return SPILua(nls, _MakeDeleter(id));
}

LuaState::LuaState(const SPLua& spLua) {
	int id = s_index.get();
	_id = id;

	_base = spLua->getMainLS_SP();
	_RegisterNewThread(*spLua, _id);
}
LuaState::LuaState(lua_State* ls, _TagThread) {
	LuaState lsc(ls);
	int top = lsc.getTop();

	// 参照カウンタのインクリメント
	lsc.pushSelf();
	_Increment_Th(lsc);
	// メインスレッドのポインタを取得、参照カウンタをインクリメント
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
	// メインスレッドの登録
	push(static_cast<void*>(this));
	setGlobal(cs_mainThread);
	// スレッドリストにも加える
	_id = s_index.get();
	_RegisterNewThread(*this, _id);
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

void LuaState::push(const LCValue& v) {
	v.push(getLS());
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
	SPLua mls(getMainLS_SP());
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
void LuaState::_checkType(int idx, LuaType typ) const {
	LuaType t = type(idx);
	if(t != typ)
		throw EType(typeName(t), typeName(typ));
}
void LuaState::_CheckType(lua_State* ls, int idx, LuaType typ) {
	LuaType t = SType(ls, idx);
	if(t != typ)
		throw EType(STypeName(ls, t), STypeName(ls, typ));
}
bool LuaState::toBoolean(int idx) const {
	return ToBoolean(getLS(), idx);
}
bool LuaState::ToBoolean(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Boolean);
	return lua_toboolean(ls, idx) != 0;
}
lua_CFunction LuaState::toCFunction(int idx) const {
	return ToCFunction(getLS(), idx);
}
lua_CFunction LuaState::ToCFunction(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Function);
	return lua_tocfunction(ls, idx);
}
lua_Integer LuaState::toInteger(int idx) const {
	return ToInteger(getLS(), idx);
}
lua_Integer LuaState::ToInteger(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Number);
	return lua_tointeger(ls, idx);
}
std::pair<const char*, size_t> LuaState::toString(int idx) const {
	return ToString(getLS(), idx);
}
std::pair<const char*, size_t> LuaState::ToString(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::String);
	size_t len;
	const char* str = lua_tolstring(ls, idx, &len);
	return std::make_pair(str, len);
}
lua_Number LuaState::toNumber(int idx) const {
	return ToNumber(getLS(), idx);
}
lua_Number LuaState::ToNumber(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Number);
	return lua_tonumber(ls, idx);
}
const void* LuaState::toPointer(int idx) const {
	return ToPointer(getLS(), idx);
}
const void* LuaState::ToPointer(lua_State* ls, int idx) {
	return lua_topointer(ls, idx);
}
SPLua LuaState::ToThread(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Thread);
	return SPLua(new LuaState(lua_tothread(ls, idx), TagThread));
}
SPLua LuaState::toThread(int idx) const {
	return ToThread(getLS(), idx);
}
lua_Unsigned LuaState::toUnsigned(int idx) const {
	return ToUnsigned(getLS(), idx);
}
lua_Unsigned LuaState::ToUnsigned(lua_State* ls, int idx) {
	_CheckType(ls, idx, LuaType::Number);
	return lua_tounsignedx(ls, idx, nullptr);
}
void* LuaState::toUserData(int idx) const {
	return ToUserData(getLS(), idx);
}
void* LuaState::ToUserData(lua_State* ls, int idx) {
	try {
		_CheckType(ls, idx, LuaType::Userdata);
	} catch(const EType& e) {
		_CheckType(ls, idx, LuaType::LightUserdata);
	}
	return lua_touserdata(ls, idx);
}
LCTable LuaState::ToTable(lua_State* ls, int idx) {
	LCTable tbl;
	lua_pushnil(ls);
	while(lua_next(ls, idx) != 0) {
		// key=-2 value=-1
		tbl.emplace(SPLCValue(new LCValue(ToLCValue(ls, -2))),
					SPLCValue(new LCValue(ToLCValue(ls, -1))));
	}
	return std::move(tbl);
}
LCTable LuaState::toTable(int idx) const {
	return ToTable(getLS(), idx);
}
namespace {
	const std::function<LCValue (lua_State* ls, int idx)> c_toLCValue[LUA_NUMTAGS+1] = {
		[](lua_State* ls, int idx){ return LCValue(boost::blank()); },
		[](lua_State* ls, int idx){ return LCValue(LuaNil()); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToBoolean(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToUserData(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToNumber(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToString(ls,idx).first); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToTable(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToCFunction(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToUserData(ls,idx)); },
		[](lua_State* ls, int idx){ return LCValue(LuaState::ToThread(ls,idx)); }
	};
	const LuaType c_toLType[LUA_NUMTAGS+1] = {
		LuaType::None,
		LuaType::Nil,
		LuaType::Boolean,
		LuaType::LightUserdata,
		LuaType::Number,
		LuaType::String,
		LuaType::Table,
		LuaType::Function,
		LuaType::Userdata,
		LuaType::Thread
	};
}
LCValue LuaState::ToLCValue(lua_State* ls, int idx) {
	int typ = lua_type(ls, idx);
	return c_toLCValue[typ+1](ls, idx);
}
LCValue LuaState::toLCValue(int idx) const {
	return ToLCValue(getLS(), idx);
}
LuaType LuaState::type(int idx) const {
	return SType(getLS(), idx);
}
LuaType LuaState::SType(lua_State* ls, int idx) {
	int typ = lua_type(ls, idx);
	return c_toLType[typ+1];
}

const char* LuaState::typeName(LuaType typ) const {
	return STypeName(getLS(), typ);
}
const char* LuaState::STypeName(lua_State* ls, LuaType typ) {
	return lua_typename(ls, static_cast<int>(typ));
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
SPLua LuaState::getLS_SP() {
	return shared_from_this();
}
SPLua LuaState::getMainLS_SP() {
	if(_base)
		return _base->getMainLS_SP();
	return shared_from_this();
}
void LuaState::_checkError(int code) const {
	_CheckError(getLS(), code);
}
void LuaState::_CheckError(lua_State* ls, int code) {
	if(code != LUA_OK) {
		const char* msg = ToString(ls, -1).first;
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
	for(int i=1 ; i<=n ; i++)
		os << "[" << i << "]: " << ls.toLCValue(i) << std::endl;
	return os;
}

