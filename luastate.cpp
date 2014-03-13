#include "luaw.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <limits>
#include "sdlwrap.hpp"

namespace rs {
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
	LuaState::Reader::Reader(HRW hRW): ops(hRW.ref()), size(ops.size()) {}
	void LuaState::Reader::Read(lua_State* ls, HRW hRW, const char* chunkName, const char* mode) {
		Reader reader(hRW);
		int res = lua_load(ls, &Reader::Proc, &reader, (chunkName ? chunkName : ""), mode);
		LuaState::_CheckError(ls, res);
	}
	const char* LuaState::Reader::Proc(lua_State* ls, void* data, size_t* size) {
		auto* self = reinterpret_cast<Reader*>(data);
		auto remain = self->size;
		if(remain > 0) {
			constexpr decltype(remain) BLOCKSIZE = 2048,
										MAX_BLOCK = 4;
			int nb, blocksize;
			if(remain <= BLOCKSIZE) {
				nb = 1;
				blocksize = *size = remain;
				self->size = 0;
			} else {
				nb = std::min(MAX_BLOCK, remain / BLOCKSIZE);
				blocksize = BLOCKSIZE;
				*size = BLOCKSIZE * nb;
				self->size -= *size;
			}
			self->buff.resize(*size);
			self->ops.read(self->buff.data(), blocksize, nb);
			return reinterpret_cast<const char*>(self->buff.data());
		}
		*size = 0;
		return nullptr;
	}
	const char* LuaState::cs_defaultmode = "bt";
	LuaState::LuaState(LuaState&& ls): _base(std::move(ls._base)), _lua(std::move(ls._lua)) {}
	void LuaState::load(HRW hRW, const char* chunkName, const char* mode, bool bExec) {
		Reader::Read(getLS(), hRW, chunkName, mode);
		if(bExec) {
			// Loadされたチャンクを実行
			call(0,0);
		}
	}
	void LuaState::loadFromSource(HRW hRW, const char* chunkName, bool bExec) {
		load(hRW, chunkName, "t", bExec);
	}
	void LuaState::loadFromBinary(HRW hRW, const char* chunkName, bool bExec) {
		load(hRW, chunkName, "b", bExec);
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
	void LuaState::pushCClosure(lua_CFunction func, int nvalue) {
		lua_pushcclosure(getLS(), func, nvalue);
	}
	void LuaState::pushValue(int idx) {
		lua_pushvalue(getLS(), idx);
	}
	void LuaState::pop(int n) {
		lua_pop(getLS(), n);
	}
	int LuaState::absIndex(int idx) const {
		idx = lua_absindex(getLS(), idx);
		assert(idx >= 0);
		return idx;
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
	const char* LuaState::getUpvalue(int idx, int n) {
		return lua_getupvalue(getLS(), idx, n);
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
	void LuaState::setMetatable(int idx) {
		lua_setmetatable(getLS(), idx);
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
	const char* LuaState::setUpvalue(int funcidx, int n) {
		return lua_setupvalue(getLS(), funcidx, n);
	}
	void* LuaState::upvalueId(int funcidx, int n) {
		return lua_upvalueid(getLS(), funcidx, n);
	}
	void LuaState::upvalueJoin(int funcidx0, int n0, int funcidx1, int n1) {
		lua_upvaluejoin(getLS(), funcidx0, n0, funcidx1, n1);
	}
	bool LuaState::status() const {
		int res = lua_status(getLS());
		_checkError(res);
		return res != 0;
	}
	void LuaState::_checkType(int idx, LuaType typ) const {
		LuaType t = type(idx);
		if(t != typ) {
			std::string tmp0(typeName(t)),
						tmp1(typeName(typ));
			throw EType(tmp0.c_str(), tmp1.c_str());
		}
	}
	void LuaState::_CheckType(lua_State* ls, int idx, LuaType typ) {
		LuaType t = SType(ls, idx);
		if(t != typ) {
			std::string tmp0(STypeName(ls, t)),
						tmp1(STypeName(ls, typ));
			throw EType(tmp0.c_str(), tmp1.c_str());
		}
	}
	bool LuaState::toBoolean(int idx) const {
		return LCV<bool>()(idx, getLS());
	}
	lua_CFunction LuaState::toCFunction(int idx) const {
		return LCV<lua_CFunction>()(idx, getLS());
	}
	lua_Integer LuaState::toInteger(int idx) const {
		return LCV<lua_Integer>()(idx, getLS());
	}
	std::string LuaState::toString(int idx) const {
		return LCV<std::string>()(idx, getLS());
	}
	std::string LuaState::cnvString(int idx) {
		getGlobal(luaNS::ToString);
		pushValue(idx);
		call(1,1);
		std::string ret = toString(idx);
		pop(1);
		return std::move(ret);
	}
	lua_Number LuaState::toNumber(int idx) const {
		return LCV<lua_Number>()(idx, getLS());
	}
	const void* LuaState::toPointer(int idx) const {
		return lua_topointer(getLS(), idx);
	}
	SPLua LuaState::toThread(int idx) const {
		return LCV<SPLua>()(idx, getLS());
	}
	lua_Unsigned LuaState::toUnsigned(int idx) const {
		return LCV<lua_Unsigned>()(idx, getLS());
	}
	void* LuaState::toUserData(int idx) const {
		return LCV<void*>()(idx, getLS());
	}
	LCTable LuaState::toTable(int idx) const {
		return LCV<LCTable>()(idx, getLS());
	}

	LCValue LuaState::toLCValue(int idx) const {
		return LCV<LCValue>()(idx, getLS());
	}
	LuaType LuaState::type(int idx) const {
		return SType(getLS(), idx);
	}
	LuaType LuaState::SType(lua_State* ls, int idx) {
		int typ = lua_type(ls, idx);
		return static_cast<LuaType>(typ);
	}

	const char* LuaState::typeName(LuaType typ) const {
		return STypeName(getLS(), typ);
	}
	const char* LuaState::STypeName(lua_State* ls, LuaType typ) {
		return lua_typename(ls,
				static_cast<int>(typ));
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
	SPLua LuaState::GetMainLS_SP(lua_State* ls) {
		lua_getglobal(ls, cs_mainThread.c_str());
		void* ptr = lua_touserdata(ls, -1);
		SPLua sp = reinterpret_cast<LuaState*>(ptr)->shared_from_this();
		lua_pop(ls, 1);
		return std::move(sp);
	}
	void LuaState::_checkError(int code) const {
		_CheckError(getLS(), code);
	}
	void LuaState::_CheckError(lua_State* ls, int code) {
		if(code != LUA_OK) {
			const char* msg = LCV<const char*>()(-1, ls);
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
}

