#include "luaw.hpp"
#include <sstream>
#include <limits>

namespace rs {
	bool LuaNil::operator == (LuaNil) const {
		return true;
	}
	std::ostream& operator << (std::ostream& os, const LCTable& lct) {
		return LCV<LCTable>()(os, lct);
	}
	// ------------------- LCValue -------------------
	// --- LCV<boost::blank> = LUA_TNONE
	void LCV<boost::blank>::operator()(lua_State* ls, boost::blank) const {
		assert(false); }
	boost::blank LCV<boost::blank>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		assert(false); }
	std::ostream& LCV<boost::blank>::operator()(std::ostream& os, boost::blank) const {
		return os << "(none)"; }
	LuaType LCV<boost::blank>::operator()() const {
		return LuaType::LNone; }

	// --- LCV<LuaNil> = LUA_TNIL
	void LCV<LuaNil>::operator()(lua_State* ls, LuaNil) const {
		lua_pushnil(ls); }
	LuaNil LCV<LuaNil>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Nil);
		return LuaNil(); }
	std::ostream& LCV<LuaNil>::operator()(std::ostream& os, LuaNil) const {
		return os << "(nil)"; }
	LuaType LCV<LuaNil>::operator()() const {
		return LuaType::Nil; }

	// --- LCV<bool> = LUA_TBOOL
	void LCV<bool>::operator()(lua_State* ls, bool b) const {
		lua_pushboolean(ls, b); }
	bool LCV<bool>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Boolean);
		return lua_toboolean(ls, idx) != 0; }
	std::ostream& LCV<bool>::operator()(std::ostream& os, bool b) const {
		return os << std::boolalpha << b; }
	LuaType LCV<bool>::operator()() const {
		return LuaType::Boolean; }

	// --- LCV<const char*> = LUA_TSTRING
	void LCV<const char*>::operator()(lua_State* ls, const char* c) const {
		lua_pushstring(ls, c); }
	const char* LCV<const char*>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		return lua_tostring(ls, idx); }
	std::ostream& LCV<const char*>::operator()(std::ostream& os, const char* c) const {
		return os << c; }
	LuaType LCV<const char*>::operator()() const {
		return LuaType::String; }

	// --- LCV<std::string> = LUA_TSTRING
	void LCV<std::string>::operator()(lua_State* ls, const std::string& s) const {
		lua_pushlstring(ls, s.c_str(), s.length()); }
	std::string LCV<std::string>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::String);
		size_t len;
		const char* c =lua_tolstring(ls, idx, &len);
		return std::string(c, len); }
	std::ostream& LCV<std::string>::operator()(std::ostream& os, const std::string& s) const {
		return os << s; }
	LuaType LCV<std::string>::operator()() const {
		return LuaType::String; }

	// --- LCV<Int_MainT> = LUA_TNUMBER
	void LCV<Int_MainT>::operator()(lua_State* ls, Int_MainT i) const {
		lua_pushinteger(ls, i); }
	Int_MainT LCV<Int_MainT>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tointeger(ls, idx); }
	std::ostream& LCV<Int_MainT>::operator()(std::ostream& os, Int_MainT i) const {
		return os << i; }
	LuaType LCV<Int_MainT>::operator()() const {
		return LuaType::Number; }

	// --- LCV<UInt_MainT> = LUA_TNUMBER
	void LCV<UInt_MainT>::operator()(lua_State* ls, UInt_MainT i) const {
		lua_pushunsigned(ls, i); }
	UInt_MainT LCV<UInt_MainT>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tounsigned(ls, idx); }
	std::ostream& LCV<UInt_MainT>::operator()(std::ostream& os, UInt_MainT i) const {
		return os << i; }
	LuaType LCV<UInt_MainT>::operator()() const {
		return LuaType::Number; }

	// --- LCV<double> = LUA_TNUMBER
	void LCV<double>::operator()(lua_State* ls, double f) const {
		lua_pushnumber(ls, f); }
	double LCV<double>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Number);
		return lua_tonumber(ls, idx); }
	std::ostream& LCV<double>::operator()(std::ostream& os, double f) const {
		return os << f; }
	LuaType LCV<double>::operator()() const {
		return LuaType::Number; }

	// --- LCV<SPLua> = LUA_TTHREAD
	void LCV<SPLua>::operator()(lua_State* ls, const SPLua& sp) const {
		sp->pushSelf();
		lua_xmove(sp->getLS(), ls, 1);
	}
	SPLua LCV<SPLua>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Thread);
		return SPLua(new LuaState(lua_tothread(ls, idx), LuaState::TagThread));
	}
	std::ostream& LCV<SPLua>::operator()(std::ostream& os, const SPLua& sp) const {
		return os;
	}
	LuaType LCV<SPLua>::operator()() const {
		return LuaType::Thread; }

	// --- LCV<void*> = LUA_TLIGHTUSERDATA
	void LCV<void*>::operator()(lua_State* ls, const void* ud) const {
		lua_pushlightuserdata(ls, const_cast<void*>(ud)); }
	void* LCV<void*>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
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
	lua_CFunction LCV<lua_CFunction>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState::_CheckType(ls, idx, LuaType::Function);
		return lua_tocfunction(ls, idx);
	}
	std::ostream& LCV<lua_CFunction>::operator()(std::ostream& os, lua_CFunction f) const {
		return os << "(function)" << std::hex << reinterpret_cast<uintptr_t>(f); }
	LuaType LCV<lua_CFunction>::operator()() const {
		return LuaType::Function; }

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
		if(itr == spm->end())
			return boost::get<SPLCTable>(itr->second);

		// 循環参照対策で先にエントリを作っておく
		SPLCTable ret(new LCTable());
		spm->emplace(ptr, ret);
		idx = lsc.absIndex(idx);
		lsc.push(LuaNil());
		while(lsc.next(idx) != 0) {
			// key=-2 value=-1
			ret->emplace(lsc.toLCValue(-2, spm), lsc.toLCValue(-1, spm));
			// valueは取り除きkeyはlua_nextのために保持
			lsc.pop(1);
		}
		return std::move(ret);
	}
	//TODO: アドレスを出力しても他のテーブルの区別がつかずあまり意味がないので改善する(出力階層の制限した上で列挙など)
	std::ostream& LCV<LCTable>::operator()(std::ostream& os, const LCTable& t) const {
		return os << "(table)" << std::hex << reinterpret_cast<uintptr_t>(&t); }
	LuaType LCV<LCTable>::operator()() const {
		return LuaType::Table; }

	// --- LCV<LCValue>
	namespace {
		const std::function<LCValue (lua_State* ls, int idx, LPointerSP* spm)> c_toLCValue[LUA_NUMTAGS+1] = {
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(boost::blank()); },
			[](lua_State* ls, int idx, LPointerSP* spm){ return LCValue(LuaNil()); },
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
	LValueG LCV<LValueG>::operator()(int idx, lua_State* ls, LPointerSP* spm) const {
		LuaState lsc(ls);
		lsc.pushValue(idx);
		SPLua sp = lsc.getMainLS_SP();
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
			LuaType operator()(const SPLua& sp) const {
				return LCV<SPLua>()(); }
			template <class T>
			LuaType operator()(const std::shared_ptr<T>& sp) const {
				return LCV<T>()(); }
		};
		struct PrintVisitor : boost::static_visitor<std::ostream&> {
			std::ostream& _os;
			PrintVisitor(std::ostream& os): _os(os) {}
			template <class T>
			std::ostream& operator()(const T& t) const {
				return LCV<T>()(_os, t); }
			std::ostream& operator()(const SPLua& sp) const {
				return LCV<SPLua>()(_os, sp); }
			template <class T>
			std::ostream& operator()(const std::shared_ptr<T>& sp) const {
				return LCV<T>()(_os, *sp); }
		};
	}
	LCValue::LCValue(): LCVar(boost::blank()) {}
	LCValue::LCValue(const LCValue& lc): LCVar(static_cast<const LCVar&>(lc)) {}
	LCValue::LCValue(LCValue&& lcv): LCVar(std::move(static_cast<LCVar&>(lcv))) {}
	bool LCValue::operator == (const LCValue& lcv) const {
		return static_cast<const LCVar&>(*this) == static_cast<const LCVar&>(lcv);
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
		return boost::apply_visitor(PrintVisitor(os), *this);
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
		assert(lua_gettop(_ls) == _pos);
		lua_pop(_ls, 1);
	}
	void LV_Stack::_init(lua_State* ls) {
		_ls = ls;
		_pos = lua_gettop(_ls);
		Assert(Trap, _pos > 0)
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
}

