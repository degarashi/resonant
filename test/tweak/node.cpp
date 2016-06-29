#include "test/tweak/node.hpp"
#include "test/tweak/drawer.hpp"
#include "test/tweak/value.hpp"
#include "../../font.hpp"
#include "stext.hpp"

namespace tweak {
	// ------------------- INode -------------------
	INode::INode(rs::CCoreID cid, const Name& name):
		_name(name),
		_text(mgr_text.createText(cid, name))
	{}
	const Name& INode::getName() const {
		return _name;
	}
	void INode::sortAlphabetically() {
		sortChild(
			[](const auto& a, const auto& b) {
				return a->getName() < b->getName();
			},
			true
		);
	}
	void INode::OnChildRemove(const node_t* self, const node_t::SP&) {
		static_cast<const INode*>(self)->_setRefreshSize();
	}
	void INode::OnChildAdded(const node_t* self, const node_t::SP&) {
		static_cast<const INode*>(self)->_setRefreshSize();
	}
	INode::node_t::SP INode::rewindRange(Size& size, const bool bChk) const {
		if(bChk) {
			if(auto ret = _rewindRange(size))
				return ret;
		} else
			size -= _getThisSize();
		if(size > 0) {
			if(auto ps = getPrevSibling())
				return ps->rewindRange(size, true);
			if(auto p = getParent())
				return p->rewindRange(size, false);
		}
		return const_cast<INode*>(this)->shared_from_this();
	}
	// ------------------- Node -------------------
	Node::Node(rs::CCoreID cid, const Name& name):
		INode(cid, name),
		_expanded(true),
		_bRefl(true)
	{
		_mark[0] = mgr_text.createText(cid, "-");
		_mark[1] = mgr_text.createText(cid, ">");
	}
	void Node::_setRefreshSize() const {
		_bRefl = true;
	}
	INode::Size Node::_getThisSize() const {
		return 1;
	}
	INode::Size Node::_getCachedSize() const {
		if(_bRefl) {
			_bRefl = false;
			Size s = _getThisSize();
			if(isExpanded()) {
				iterateChild(
					[&s](auto& nd){
						s += nd->_getCachedSize();
						return true;
					}
				);
			}
			_cached_size = s;
		}
		return _cached_size;
	}
	bool Node::expand(const bool b) {
		const auto br = _expanded ^ b;
		_bRefl |= br;
		_expanded = b;
		return br;
	}
	bool Node::isExpanded() const {
		return _expanded;
	}
	bool Node::isNode() const {
		return true;
	}
	void Node::setPointer(Value_UP) {}
	spn::SizeF Node::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		const auto sz = _text->getSize();
		d.drawRectBoth({
			offset.x,
			offset.x+sz.width,
			offset.y,
			offset.y-sz.height
		}, {Color::Node});
		// ノード名
		d.drawText(offset, _text, {Color::Node});
		// ノードを開いた印
		d.drawText(offset + Vec2(sz.width + 8, 0), _mark[_expanded], {Color::Node});
		return {sz.width+16, unit.y};
	}
	std::ostream& Node::write(std::ostream& s) const {
		return s;
	}
	spn::SizeF Node::drawInfo(const Vec2&, const Vec2&, const STextPack&, Drawer&) const { return {}; }
	INode::node_t::SP Node::_rewindRange(Size& size) const {
		const auto cs = _getCachedSize();
		if(isExpanded()) {
			if(size <= cs) {
				// このノードを精査
				auto cur = getChild();
				while(auto next = cur->getSibling())
					cur = next;
				for(;;) {
					if(auto ret = cur->rewindRange(size))
						return ret;
					if(auto next = cur->getPrevSibling())
						cur = next;
					else
						break;
				}
				AssertP(Trap, size <= _getThisSize())
				return const_cast<Node*>(this)->shared_from_this();
			}
			size -= cs;
		} else {
			size -= _getThisSize();
			if(size <= 0)
				return const_cast<Node*>(this)->shared_from_this();
		}
		return nullptr;
	}
	void Node::reset() {
		iterateChild(
			[](auto& nd){
				nd->reset();
				return true;
			}
		);
	}
	void Node::setAsInitial() {
		iterateChild(
			[](auto& nd){
				nd->setAsInitial();
				return true;
			}
		);
	}

	// ------------------- Entry -------------------
	Entry::Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def):
		INode(cid, name),
		_target(target),
		_value(def->defvalue->clone()),
		_initialValue(def->defvalue->clone()),
		_def(def)
	{}
	bool Entry::expand(const bool /*b*/) {
		return false;
	}
	bool Entry::isExpanded() const {
		return false;
	}
	bool Entry::isNode() const {
		return false;
	}
	spn::SizeF Entry::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		const auto sz = _text->getSize();
		// (本当はnameの描画もする)
		d.drawText(offset, _text, {Color::Node});
		auto ofs = offset;
		ofs.x += sz.width + 8;
		return _value->draw(ofs, unit, d);
	}
	void Entry::_setRefreshSize() const {}
	INode::Size Entry::_getThisSize() const {
		return 1;
	}
	INode::Size Entry::_getCachedSize() const {
		return _getThisSize();
	}
	spn::SizeF Entry::drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const {
		const spn::SizeF s = _value->drawInfo(offset, unit, st, d);
		const Color color{Color::Node};
		auto ofs = offset;
		ofs.y += s.height;
		int mx = s.width;
		// "Initial" InitialValue
		int tx = d.drawTexts(ofs, color, st.getText(STextPack::Initial), unit.x);
		tx += 8;
		tx += _initialValue->draw({ofs.x+tx, ofs.y}, unit, d).width;
		mx = std::max<int>(mx, tx);
		ofs.y += unit.y;

		// Overall-Rect (compounds those texts)
		d.drawRect(
			{
				offset.x, offset.x+mx,
				offset.y, ofs.y
			},
			true,
			{Color::Node}
		);
		return {mx, ofs.y - offset.y};
	}
	void Entry::setPointer(Value_UP v) {
		_value = std::move(v);
		_applyValue();
	}
	void Entry::loadDefine(const LValueS& tbl) {
		_value->loadDefine(tbl);
		_initialValue->loadDefine(tbl);
		_applyValue();
	}
	void Entry::loadValue(const LValueS& v) {
		_value->loadValue(v);
		_initialValue->loadValue(v);
		_applyValue();
	}
	rs::LCValue Entry::get() const {
		return _value->get();
	}
	void Entry::increment(const float inc, const int index) {
		_value->increment(inc, index);
		_applyValue();
	}
	void Entry::_applyValue() {
		if(auto h = _target.getManager()->lock(_target))
			_def->apply.call(h, get());
	}
	std::ostream& Entry::write(std::ostream& s) const {
		s <<  getName() << " = ";
		return _value->write(s);
	}
	INode::node_t::SP Entry::_rewindRange(Size& size) const {
		size -= _getCachedSize();
		if(size <= 0)
			return const_cast<Entry*>(this)->shared_from_this();
		return nullptr;
	}
	void Entry::reset() {
		_value = _initialValue->clone();
	}
	void Entry::setAsInitial() {
		_initialValue = _value->clone();
	}
}
