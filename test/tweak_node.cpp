#include "tweak.hpp"

// ------------------- Tweak::INode -------------------
Tweak::INode::INode(rs::CCoreID cid, const Name& name):
	_name(name),
	_text(mgr_text.createText(cid, name))
{}
const Tweak::Name& Tweak::INode::getName() const {
	return _name;
}

// ------------------- Tweak::Node -------------------
Tweak::Node::Node(rs::CCoreID cid, const Name& name):
	INode(cid, name),
	_expanded(true)
{
	_mark[0] = mgr_text.createText(cid, "-");
	_mark[1] = mgr_text.createText(cid, ">");
}
bool Tweak::Node::expand(const bool b) {
	const auto br = _expanded ^ b;
	_expanded = b;
	return br;
}
bool Tweak::Node::isExpanded() const {
	return _expanded;
}
bool Tweak::Node::isNode() const {
	return true;
}
void Tweak::Node::setPointer(Value_UP) {}
int Tweak::Node::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
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

// ------------------- Tweak::Entry -------------------
Tweak::Entry::Entry(rs::CCoreID cid, const Name& name, spn::WHandle target, const Define_SP& def):
	INode(cid, name),
	_target(target),
	_value(def->defvalue->clone()),
	_def(def)
{}
bool Tweak::Entry::expand(const bool /*b*/) {
	return false;
}
bool Tweak::Entry::isExpanded() const {
	return false;
}
bool Tweak::Entry::isNode() const {
	return false;
}
int Tweak::Entry::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
	const auto sz = _text->getSize();
	if(!d.checkDraw(this, {offset.x, offset.x+sz.width,
							offset.y, offset.y-sz.height}))
		return 0;
	// 本当はnameの描画もする
	d.drawText(offset, _text, {Color::Node});
	auto ofs = offset;
	ofs.x += sz.width + 8;
	return _value->draw(ofs, unit, d);
}
void Tweak::Entry::setPointer(Value_UP v) {
	_value = std::move(v);
	_applyValue();
}
void Tweak::Entry::set(const LValueS& v, const bool bStep) {
	_value->set(v, bStep);
	_applyValue();
}
rs::LCValue Tweak::Entry::get() const {
	return _value->get();
}
void Tweak::Entry::increment(const float inc, const int index) {
	_value->increment(inc, index);
	_applyValue();
}
void Tweak::Entry::_applyValue() {
    if(auto h = _target.getManager()->lock(_target))
		_def->apply.call(h, get());
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Tweak::INode::SP, INodeSP, NOTHING, (use_count))
