#include "tweak.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"
#include "../font.hpp"

// ---------------- Tweak::Color ----------------
namespace {
	const rs::util::ColorA cs_color[Tweak::Color::_Num] = {
		{{1,1,1}},
		{{1,1,0.5f}},
		{{1,0.5f,1}},
		{{0.5f,1,1}},
		{{0.5f, 0.5f, 1}},
		{{1,1,1}}
	};
}
const rs::util::ColorA& Tweak::Color::get() const {
	return cs_color[c];
}
// ---------------- Tweak::Drawer ----------------
namespace {
	using GId = rs::IEffect::GlxId;
	const rs::IdValue
		T_TweakRect			= GId::GenTechId("Tweak", "Rect"),
		T_TweakText			= GId::GenTechId("Tweak", "Text");
	const rs::IdValue T_Text = GId::GenTechId("Text", "Default");
}
Tweak::Drawer::Drawer(const RectF& range, const Vec2& offset,
						const INode::SP& cur, rs::IEffect& e):
	_effect(e),
	_range(range),
	_cursor(cur.get()),
	_offset(offset),
	_cursorAt(0)
{
	_text.setDepth(0.f);
}
void Tweak::Drawer::drawRect(const RectF& rect, const bool bWireframe, const Color color) {
	_effect.setTechPassId(T_TweakRect);
	_rect.setWireframe(bWireframe);
	_rect.setScale({rect.width(), rect.height()});
	_rect.setOffset({rect.x0, rect.y0});
	_rect.setColor(color.get().asVec3());
	_rect.setAlpha(bWireframe ? 0.25f : 0.15f);
	_rect.draw(_effect);
}
void Tweak::Drawer::drawRectBoth(const RectF& rect, const Color color) {
	drawRect(rect, true, color);
	drawRect(rect, false, color);
}
void Tweak::Drawer::drawText(const Vec2& offset, rs::HText hText, const Color color) {
	_effect.setTechPassId(T_Text);
	_text.setText(hText);
	_text.setWindowOffset(offset);
	_text.refColor() = color.get();
	_text.draw(_effect);
}
void Tweak::Drawer::drawTextVCenter(const RectF& rect, rs::HText hText, const Color color) {
	const auto sz = hText->getSize();
	drawText(
		{
			rect.x0,
			(rect.y0-sz.height)/2
		},
		hText,
		color
	);
}
#include "boomstick/geom2D.hpp"
bool Tweak::Drawer::checkDraw(const INode* s, const RectF& r) {
	if(s == _cursor)
		_cursorAt = {r.x0, r.y0};
	auto makeAB = [](const RectF& r){
		return boom::geo2d::AABB(
				{r.x0, r.y0},
				{r.x1, r.y1}
		);
	};
	return makeAB(r).hit(makeAB(_range));
}
const spn::Vec2& Tweak::Drawer::getCursorAt() const {
	return _cursorAt;
}
