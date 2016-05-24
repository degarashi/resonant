#include "tweak.hpp"
#include "../glresource.hpp"

namespace {
	const std::string g_fontName("IPAGothic");
}
Tweak::Tweak(const std::string& rootname, const int tsize):
	_offset(0),
	_tsize(tsize),
	_indent(tsize*2)
{
	_cid = mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, tsize, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel));
	_root = std::make_shared<Node>(_cid, rootname);
	_cursor = _root;
	setStateNew<St_Cursor>();
}
namespace {
	void PrintLuaValue(const rs::LValueS& lv) {
		rs::LuaState ls(lv.getLS());
		ls.getGlobal("RS");
		ls.getField(-1, "PrintValue");
		ls.push("");
		lv.prepareValue(lv.getLS());
		ls.call(2);
		ls.pop(1);
	}
}
Tweak::DefineV Tweak::_loadDefine(const LValueS& d) {
	LValueS src(d["_variable"]);
	DefineV v;
	src.iterateTable([cid=_cid, &v](rs::LuaState& ls){
		auto name = ls.toString(-2);
		{
			LValueS tbl(ls.getLS());
			const auto manip = LValueS(tbl["manip"]).toValue<std::string>();
			const int sz = FVec4::LoadFromLua(LValueS(tbl["default"])).nValue;
			Value_UP defval;
			if(manip == "linear") {
				defval = std::make_unique<LinearValue>(cid, sz);
			} else if(manip == "log") {
				defval = std::make_unique<LogValue>(cid, sz);
			} else if(manip == "dir2d") {
				defval = std::make_unique<Dir2D>(cid);
			} else {
				Assert(Trap, manip=="dir3d")
				defval = std::make_unique<Dir3D>(cid);
			}
			defval->set(LValueS(tbl["default"]), false);
			defval->set(LValueS(tbl["step"]), true);

			LValueS apl(tbl["apply"]);
			if(apl.type() == rs::LuaType::String) {
				const auto str = (boost::format(
				   "return function(self, value)\
						self:%1%(value)\
					end"
				) % apl.toString()).str();
				auto hlSrc = mgr_rw.fromConstMem(str.c_str(), str.size());
				ls.loadFromSource(hlSrc, nullptr, false);
				ls.call(0, 1);
				LValueS apl_t(ls.getLS());
				apl = apl_t;
			}
			apl.prepareValue(apl.getLS());
			LValueG lg(apl.getLS());
			Define_SP defsp(new Define{name, std::move(defval), lg});
			v.emplace_back(std::move(defsp));
		}
		ls.push(0);
	});
	return v;
}
const Tweak::DefineV& Tweak::_checkDefine(const Name& name, lua_State* ls) {
	auto itr = _define.find(name);
	if(itr == _define.end()) {
		rs::LuaState lsc(ls);
		lsc.getGlobal(name);
		Assert(Trap, lsc.type(-1) == rs::LuaType::Table)
		itr = _define.emplace(name, _loadDefine(ls)).first;
	}
	return itr->second;
}
const Tweak::Define_SP& Tweak::_getDefine(const Name& objname, const Name& entname, lua_State* ls) {
	const auto& defv = _checkDefine(objname, ls);
	const auto itr = std::find_if(defv.begin(), defv.end(), [&](auto& e){ return e->name==entname; });
	Assert(Throw, itr!=defv.end(), "definition %1%::%2% is not found.", objname, entname)
	return *itr;
}
void Tweak::setCursorAxis(HAct hX, HAct hY) {
	_haX = hX;
	_haY = hY;
}
void Tweak::setSwitchButton(HAct hSw) {
	_haSw = hSw;
}
void Tweak::setIncrementAxis(HAct hC, HAct hX, HAct hY, HAct hZ, HAct hW) {
	_haCont = hC;
	_haInc[0] = hX;
	_haInc[1] = hY;
	_haInc[2] = hZ;
	_haInc[3] = hW;
}
Tweak::INode::SP Tweak::loadValue(spn::SHandle obj, const std::string& file, const std::string& name, const rs::SPLua& ls) {
	if(auto rw = mgr_path.getRW(rs::luaNS::ScriptResourceEntry, file, nullptr)) {
		std::cout << *ls << std::endl;
		const int nRet = ls->loadFromSource(rw);
		Assert(Trap, nRet==1, "invalid tweakable value file")
		std::cout << *ls << std::endl;
		rs::LValueS lv(ls->getLS());
		auto sp = makeGroup(name);
		lv.iterateTable([&sp, this, obj](auto& ls){
			// [Key][Value]
			ls.pushValue(-2);
			ls.pushValue(-2);
			// [Key][Value][Key][Value]
			sp->addChild(makeEntry(ls.toString(-2), obj, LValueS(ls.getLS())));
			// [Key][Value][Key]
			ls.pop(1);
		});
		return sp;
	}
	return nullptr;
}
Tweak::INode::SP Tweak::makeGroup(const Name& name) {
	return std::make_shared<Node>(_cid, name);
}
Tweak::INode::SP Tweak::makeEntry(const Name& entname, spn::SHandle obj, const LValueS& v) {
	// オブジェクトハンドルから名前を特定
	const auto& objname = obj.getResourceName();
	try {
		const auto& def = _getDefine(objname, entname, v.getLS());
		auto value = def->defvalue->clone();
		value->set(v, false);
		return std::make_shared<Entry>(_cid, entname, obj.weak(), std::move(value), def);
	} catch(const std::runtime_error& e) {
		Assert(Warn, false, e.what())
	}
	return Entry_SP();
}
void Tweak::insertNext(const INode::SP& sp) {
	_cursor->addSibling(sp);
}
void Tweak::insertChild(const INode::SP& sp) {
	_cursor->addChild(sp);
}
void Tweak::setValue(const LValueS& v) {
	_cursor->set(v, false);
}
void Tweak::increment(const float inc, const int index) {
	_cursor->increment(inc, index);
}
std::pair<Tweak::INode::SP,int> Tweak::_remove(const INode::SP& sp, const bool delNode) {
	if(sp == _root)
		return std::make_pair(sp, 0);

	int count = 1;
	auto p = sp->getParent();
	p->removeChild(sp);
	if(delNode && !p->getChild()) {
		int tc;
		std::tie(p, tc) = _remove(p, true);
		count += tc;
	}
	return std::make_pair(p, count);
}
int Tweak::remove(const bool delNode) {
	int count;
	std::tie(_cursor, count) = _remove(_cursor, delNode);
	return count;
}
int Tweak::removeObj(rs::HObj obj, const bool delNode) {
	int count = 0;
	auto itr = _obj2ent.find(obj);
	if(itr != _obj2ent.end()) {
		for(auto& e : itr->second) {
			int tc;
			std::tie(std::ignore, tc) = _remove(e, delNode);
			count += tc;
		}
		_obj2ent.erase(itr);
	}
	return count;
}
void Tweak::setFontSize(const int tsize) {
	_tsize = tsize;
}
void Tweak::setOffset(const Vec2& ofs) {
	_offset = ofs;
}
void Tweak::setIndent(const float w) {
	_indent = w;
}
bool Tweak::expand() {
	return _cursor->expand(true);
}
bool Tweak::fold() {
	return _cursor->expand(false);
}
namespace {
	template <class T, class F>
	bool SetAndBoolean(T& cur, F&& f) {
		auto res = f();
		if(res) {
			if(cur != res) {
				cur = res;
				return true;
			}
		}
		return false;
	}
}
bool Tweak::up() {
	return SetAndBoolean(_cursor, [this](){return _cursor->getParent();});
}
bool Tweak::down() {
	if(_cursor->isExpanded())
		return SetAndBoolean(_cursor, [this](){return _cursor->getChild();});
	return false;
}
bool Tweak::next() {
	return SetAndBoolean(_cursor, [this](){return _cursor->getSibling();});
}
bool Tweak::prev() {
	return SetAndBoolean(_cursor, [this](){ return _cursor->getPrevSibling(false);});
}
void Tweak::setCursor(const INode::SP& s) {
	_cursor = s;
}
const Tweak::INode::SP& Tweak::getCursor() const {
	return _cursor;
}
const Tweak::INode::SP& Tweak::getRoot() const {
	return _root;
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, Tweak, Tweak, "DrawableObj",
	NOTHING,
	(loadValue)
	(setDrawPriority)
	(setCursorAxis)(setIncrementAxis)(setSwitchButton)
	(makeGroup)(makeEntry)(setValue)(increment)(remove)(removeObj)
	(setFontSize)(setOffset)
	(insertNext)(insertChild)
	(expand)(fold)(up)(down)(next)(prev)(setCursor)(getCursor)(getRoot),
	(const std::string&)(int))

#include "../systeminfo.hpp"
#include "../input.hpp"
struct Tweak::St_Base : StateT<St_Base> {
	void onDraw(const Tweak& self, rs::IEffect& e) const override {
		// ノード描画
		const auto sz = mgr_info.getScreenSize();
		Drawer drawer(
			{
				0.f, sz.width,
				0.f, sz.height
			},
			self._offset,
			self._cursor,
			e
		);
		self._root->draw(
			{0, 0},
			{self._indent, self._tsize+4},
			drawer
		);
		// カーソル描画
		const auto& at = drawer.getCursorAt();
		constexpr float ofsx = 32,
						ofsy = 2;
		drawer.drawRectBoth({
			at.x-ofsx,
			at.x-ofsx/2,
			at.y-ofsy,
			at.y+self._tsize+ofsy
		}, {Color::Cursor});
	}
};
struct Tweak::St_Value : St_Base {
	St_Value() {
		std::cout << "St_Value" << std::endl;
	}
	void onUpdate(Tweak& self) override {
		// インクリメント入力判定
		const std::function<int (HAct)> f = self._haCont->isKeyPressing() ?
			[](HAct a){
				return a->getKeyValueSimplified();
			}:
			[](HAct a){
				return a->getKeyValueSimplifiedOnce();
			};
		spn::Vec4 diff(0);
		for(int i=0 ; i<static_cast<int>(countof(self._haInc)) ; i++) {
			const auto d = f(self._haInc[i]);
			if(d != 0)
				self._cursor->increment(d, i);
		}
		St_Base::onUpdate(self);

		// モード切り替え判定
		if(self._haSw->isKeyPressed())
			self.setStateNew<St_Cursor>();
	}
};
struct Tweak::St_Cursor : St_Base {
	St_Cursor() {
		std::cout << "St_Cursor" << std::endl;
	}
	void onUpdate(Tweak& self) override {
		// カーソル移動入力判定
		const auto vy = self._haY->getKeyValueSimplifiedOnce();
		// エントリ間
		if(vy > 0) {
			// 上へ
			self.prev();
		} else if(vy < 0) {
			// 下へ
			self.next();
		} else {
			// Contが押されている間はノードの開閉とする
			const bool bc = self._haCont->isKeyPressing();
			const auto vx = self._haX->getKeyValueSimplifiedOnce();
			// 階層間
			if(vx > 0) {
				if(bc) {
					// ノード展開
					self.expand();
				} else {
					// 下の階層へ
					self.down();
				}
			} else if(vx < 0) {
				if(bc) {
					// ノードを閉じる
					self.fold();
				} else {
					// 上の階層へ
					self.up();
				}
			}
		}
		St_Base::onUpdate(self);

		// モード切り替え判定
		if(self._haSw->isKeyPressed())
			self.setStateNew<St_Value>();
	}
};