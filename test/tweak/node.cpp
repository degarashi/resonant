#include "test/tweak/node.hpp"
#include "test/tweak/drawer.hpp"
#include "test/tweak/value.hpp"
#include "../../font.hpp"

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
	// ------------------- Node -------------------
	Node::Node(rs::CCoreID cid, const Name& name):
		INode(cid, name),
		_expanded(true)
	{
		_mark[0] = mgr_text.createText(cid, "-");
		_mark[1] = mgr_text.createText(cid, ">");
	}
	bool Node::expand(const bool b) {
		const auto br = _expanded ^ b;
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
	int Node::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		const auto sz = _text->getSize();
		if(!d.checkDraw(this, {offset.x, offset.x+sz.width,
								offset.y, offset.y-sz.height}))
			return 0;
		d.drawRectBoth({
			offset.x,
			offset.x+sz.width,
			offset.y,
			offset.y-sz.height
		}, {Color::Node});
		// X軸にオフセットを加えて子ノードを描画
		d.drawText(offset, _text, {Color::Node});
		// ノードを開いた印
		d.drawText(offset + Vec2(sz.width + 8, 0), _mark[_expanded], {Color::Node});
		int count = 1;
		if(_expanded) {
			if(auto sp = getChild()) {
				auto ofs = offset + unit;
				do {
					const int th = sp->draw(ofs, unit, d);
					ofs.y += th * unit.y;
					++count;
				} while((sp = sp->getSibling()));
			}
		}
		return count;
	}
	std::ostream& Node::write(std::ostream& s) const {
		return s;
	}
	void Node::drawInfo(const Vec2& /*offset*/, const Vec2& /*unit*/, Drawer& /*d*/) const {}

	// ------------------- Entry -------------------
	Entry::Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def):
		INode(cid, name),
		_target(target),
		_value(def->defvalue->clone()),
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
	int Entry::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		// 描画範囲チェック
		const auto sz = _text->getSize();
		if(!d.checkDraw(this, {offset.x, offset.x+sz.width,
								offset.y, offset.y-sz.height}))
			return 0;
		// (本当はnameの描画もする)
		d.drawText(offset, _text, {Color::Node});
		auto ofs = offset;
		ofs.x += sz.width + 8;
		return _value->draw(ofs, unit, d);
	}
	void Entry::drawInfo(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		// Type
		// d.drawText();
		// Step
		// InitialValue
	}
	void Entry::setPointer(Value_UP v) {
		_value = std::move(v);
		_applyValue();
	}
	void Entry::set(const LValueS& v, const bool bStep) {
		_value->set(v, bStep);
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
}
