#include "test/tweak/tweak.hpp"
#include "../../input.hpp"

namespace {
	const std::string g_fontName("IPAGothic");
	const std::string cs_rtname("value");
	void PrintLuaValue(const rs::LValueS& lv) {
		rs::LuaState ls(lv.getLS());
		ls.getGlobal("RS");
		ls.getField(-1, "PrintValue");
		ls.push("");
		lv.prepareValue(lv.getLS());
		ls.call(2);
		ls.pop(1);
	}
	const auto FindNode = [](const auto& node, const auto& name) -> tweak::INode::SP {
		if(auto cur = node->getChild()) {
			do {
				if(cur->getName() == name)
					return cur;
				cur = cur->getSibling();
			} while(cur);
		}
		return nullptr;
	};
}
namespace tweak {
	Tweak::Tweak(const std::string& rootname, const int tsize):
		_cid(
			mgr_text.makeCoreID(
				g_fontName,
				rs::CCoreID(0, tsize, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel)
			)
		),
		_stext(_cid),
		_dparam{
			{0,0},
			{-1, -1},
			float(tsize),
			float(tsize*2)
		}
	{
		_root = std::make_shared<Node>(_cid, rootname);
		_cursor = _root;
		setStateNew<St_Cursor>();
	}
	Tweak::DefineV Tweak::_LoadDefine(const LValueS& d, const rs::CCoreID cid) {
		LValueS src(d[entry::Variable]);
		DefineV v;
		src.iterateTable([cid, &v](rs::LuaState& ls){
			auto name = ls.toString(-2);
			{
				LValueS tbl(ls.getLS());
				const auto manip = LValueS(tbl[entry::Manip]).toValue<std::string>();
				const int sz = FVec4::LoadFromLua(LValueS(tbl[entry::Value])).nValue;
				Value_UP defval;
				if(manip == LinearValue::Name) {
					defval = std::make_unique<LinearValue>(cid, sz);
				} else if(manip == ExpValue::Name) {
					defval = std::make_unique<ExpValue>(cid, sz);
				} else if(manip == Dir2D::Name) {
					defval = std::make_unique<Dir2D>(cid);
				} else {
					Assert(Trap, manip==Dir3D::Name)
					defval = std::make_unique<Dir3D>(cid);
				}
				defval->loadDefine(tbl);

				LValueS apl(tbl[entry::Apply]);
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
	const Tweak::DefineV& Tweak::_getDefineV(const Name& objname, lua_State* ls) {
		auto itr = _define.find(objname);
		if(itr == _define.end()) {
			rs::LuaState lsc(ls);
			lsc.getGlobal(objname);
			Assert(Trap, lsc.type(-1) == rs::LuaType::Table)
			itr = _define.emplace(objname, _LoadDefine(ls, _cid)).first;
		}
		return itr->second;
	}
	const Define_SP& Tweak::_getDefine(const Name& objname, const Name& entname, lua_State* ls) {
		const auto& defv = _getDefineV(objname, ls);
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
	INode::SP Tweak::makeGroup(const Name& name) {
		return std::make_shared<Node>(_cid, name);
	}
	INode::SP Tweak::makeEntry(const spn::SHandle obj, const Name& ent, lua_State* ls) {
		// オブジェクトハンドルから名前を特定
		const auto& objname = obj.getResourceName();
		try {
			const auto& def = _getDefine(objname, ent, ls);
			return std::make_shared<Entry>(_cid, ent, obj.weak(), def);
		} catch(const std::runtime_error& e) {
			Assert(Warn, false, e.what())
		}
		return nullptr;
	}
	void Tweak::setEntryDefault(const INode::SP& sp, spn::SHandle obj, lua_State* ls) {
		// オブジェクトハンドルから名前を特定
		const auto& objname = obj.getResourceName();
		try {
			const auto& dv = _getDefineV(objname, ls);
			for(auto& d : dv) {
				INode::SP nd = FindNode(sp, d->name);
				if(!nd) {
					nd = makeEntry(obj, d->name, ls);
					sp->addChild(nd);
				} else
					nd->setPointer(d->defvalue->clone());
			}
		} catch(const std::runtime_error& e) {
			Assert(Warn, false, e.what())
		}
	}
	void Tweak::setEntryFromTable(const INode::SP& sp, const SHandle obj, const LValueS& tbl) {
		auto* lsp = tbl.getLS();
		tbl.iterateTable([&sp, obj, lsp, this](auto& ls){
			// [Key][Value]
			ls.pushValue(-2);
			ls.pushValue(-2);
			// [Key][Value][Key][Value]
			{
				LValueS value(ls.getLS());
				auto entname = ls.toString(-2);
				INode::SP nd = FindNode(sp, entname);
				if(!nd) {
					nd = makeEntry(obj, entname, lsp);
					sp->addChild(nd);
				}
				nd->loadValue(value);
			}
			// [Key][Value][Key]
			ls.pop(1);
		});
	}
	void Tweak::setEntryFromFile(const INode::SP& sp, SHandle obj, const std::string& file, const rs::SPLua& ls) {
		if(auto rw = mgr_path.getRW(cs_rtname, file, rs::RWops::Read, nullptr)) {
			const int nRet = ls->loadFromSource(rw);
			Assert(Trap, nRet==1, "invalid tweakable value file")
			setEntryFromTable(sp, obj, rs::LValueS(ls->getLS()));
			auto& info = _obj2info[obj];
			info.resourceName = file;
			info.entry = sp;
		}
	}
	void Tweak::insertNext(const INode::SP& sp) {
		_cursor->addSibling(sp);
	}
	void Tweak::insertChild(const INode::SP& sp) {
		_cursor->addChild(sp);
	}
	void Tweak::increment(const float inc, const int index) {
		_cursor->increment(inc, index);
	}
	std::pair<INode::SP,int> Tweak::_remove(const INode::SP& sp, const bool delNode) {
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
	void Tweak::removeObj(const SHandle obj, const bool delNode) {
		auto itr = _obj2info.find(obj);
		if(itr != _obj2info.end()) {
			_remove(itr->second.entry, delNode);
			_obj2info.erase(itr);
		}
	}
	void Tweak::setFontSize(const int tsize) {
		_dparam.tsize = tsize;
	}
	void Tweak::setOffset(const Vec2& ofs) {
		_dparam.offset = ofs;
	}
	void Tweak::setIndent(const float w) {
		_dparam.indent = w;
	}
	void Tweak::setSize(const spn::SizeF& s) {
		_dparam.size = s;
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
	const INode::SP& Tweak::getCursor() const {
		return _cursor;
	}
	const INode::SP& Tweak::getRoot() const {
		return _root;
	}
	void Tweak::saveAll() {
		for(auto& info : _obj2info) {
			save(info.first, spn::none);
		}
	}
	void Tweak::save(SHandle obj, const spn::Optional<std::string>& path) {
		auto itr = _obj2info.find(obj);
		if(itr == _obj2info.end())
			return;

		rs::HLRW rw;
		const auto f = [](auto& path){
			return mgr_path.getRW(cs_rtname, path, rs::RWops::Write, nullptr);
		};
		// リソースパスが引数に与えられていればそれを、なければロード時のリソースパスでファイルを開く
		if(path)
			rw = f(*path);
		else
			rw = f(itr->second.resourceName);
		if(!rw)
			return;
		_Save(itr->second.entry, rw);
		itr->second.entry->setAsInitial();
	}
	void Tweak::_Save(const INode::SP& ent, rs::HRW rw) {
		std::stringstream ss;
		ss << "return {" << std::endl;
		bool bF = true;
		ent->iterateDepthFirst<false>([&ss, &bF](auto& nd, const int depth){
			if(depth == 0)
				return spn::Iterate::StepIn;
			if(!bF)
				ss << ',' << std::endl;
			ss << '\t';
			bF = false;
			nd.write(ss);
			return spn::Iterate::Next;
		});
		ss << std::endl << '}' << std::endl;
		const auto str = ss.str();
		rw->write(str.c_str(), 1, str.length());
	}
}

#include "../systeminfo.hpp"
#include "../input.hpp"
#include "drawer.hpp"
namespace tweak {
	void Tweak::St_Base::onDraw(const Tweak& self, rs::IEffect& e) const {
		const auto& dp = self._dparam;
		auto sz = dp.size;
		const auto ssz = mgr_info.getScreenSize();
		if(sz.width < 0)
			sz.width = ssz.width;
		if(sz.height < 0)
			sz.height = ssz.height;
		const auto ts = dp.tsize;

		// ノード描画
		Drawer drawer(e);
		// 描画開始ノードを選択
		decltype(self._cursor) cur;
		// カーソル位置から表示範囲半分、上に遡る
		{
			int tmp = sz.height/2/dp.tsize;
			cur = self._cursor->rewindRange(tmp);
		}
		int count = 1;
		{
			auto cur2 = cur;
			while(auto p = cur2->getParent()) {
				cur2 = p;
				++count;
			}
		}
		const Vec2 unit(dp.indent, ts+4);
		Vec2 ofs = dp.offset + Vec2(count*unit.x, 0);
		while(cur) {
			// カーソル描画
			if(cur == self._cursor) {
				constexpr float ofsx = 32,
								ofsy = 2;
				const float ofsC = (St_Value::GetStateId()==getStateId()) ?
									ts/4 : 0;
				drawer.drawRectBoth({
					ofs.x - ofsx,
					ofs.x - ofsx/2,
					ofs.y - ofsy + ofsC,
					ofs.y + ts + ofsy - ofsC
				}, {Color::Cursor});
			}
			ofs.y += cur->draw(ofs, unit, drawer).height;
			// 描画範囲を超えたら停止
			if(ofs.y >= dp.offset.y + sz.height)
				break;
			INode::SP sp;
			if(cur->isExpanded() && (sp = cur->getChild())) {
				ofs.x += unit.x;
				cur = sp;
			} else if((sp = cur->getSibling())) {
				cur = sp;
			} else if((sp = cur->getParent())) {
				ofs.x -= unit.x;
				cur = sp->getSibling();
			} else
				break;
		}
		// 詳細描画
		self._cursor->drawInfo(
			{512, 0},
			unit,
			self._stext,
			drawer
		);
	}
	Tweak::St_Value::St_Value() {
		std::cout << "St_Value" << std::endl;
	}
	void Tweak::St_Value::onUpdate(Tweak& self) {
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
	Tweak::St_Cursor::St_Cursor() {
		std::cout << "St_Cursor" << std::endl;
	}
	void Tweak::St_Cursor::onUpdate(Tweak& self) {
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
}
