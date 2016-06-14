#include "test/tweak/drawer.hpp"
#include "boomstick/geom2D.hpp"
#include "../../glx_if.hpp"

namespace tweak {
	// ---------------- Drawer ----------------
	namespace {
		using GId = rs::IEffect::GlxId;
		const rs::IdValue
			T_TweakRect			= GId::GenTechId("Tweak", "Rect"),
			T_TweakText			= GId::GenTechId("Tweak", "Text");
		const rs::IdValue T_Text = GId::GenTechId("Text", "Default");
		using RectF = spn::RectF;
	}
	Drawer::Drawer(rs::IEffect& e):
		_effect(e)
	{
		_text.setDepth(0.f);
	}
	void Drawer::drawRect(const RectF& rect, const bool bWireframe, const Color color) {
		_effect.setTechPassId(T_TweakRect);
		_rect.setWireframe(bWireframe);
		_rect.setScale({rect.width(), rect.height()});
		_rect.setOffset({rect.x0, rect.y0});
		_rect.setColor(color.get().asVec3());
		_rect.setAlpha(bWireframe ? 0.25f : 0.15f);
		_rect.draw(_effect);
	}
	void Drawer::drawRectBoth(const RectF& rect, const Color color) {
		drawRect(rect, true, color);
		drawRect(rect, false, color);
	}
	int Drawer::drawText(const Vec2& offset, rs::HText hText, const Color color) {
		_effect.setTechPassId(T_Text);
		_text.setText(hText);
		_text.setWindowOffset(offset);
		_text.refColor() = color.get();
		return _text.draw(_effect);
	}
	int Drawer::drawTextVCenter(const RectF& rect, rs::HText hText, const Color color) {
		const auto sz = hText->getSize();
		return drawText(
			{
				rect.x0,
				(rect.y0-sz.height)/2
			},
			hText,
			color
		);
	}
}
