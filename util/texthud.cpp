#include "textdraw.hpp"
#include "../glx.hpp"
#include "../sys_uniform_value.hpp"
#include "../gameloophelper.hpp"

namespace rs {
	namespace util {
		// ---------------------- TextHUD ----------------------
		const IdValue TextHUD::U_Text = GLEffect::GlxId::GenUnifId("mText");
		TextHUD::TextHUD(IdValue idTech):
			Text(idTech),
			_coordType(Coord::Window),
			_offset(0),
			_scale(1),
			_depth(1)
		{}
		spn::Mat33 TextHUD::_makeMatrix() const {
			float rx, ry,
				  ox, oy,
				  diry;
			auto lkb = sharedbase.lock();
			auto s = lkb->screenSize;
			rx = spn::Rcp22Bit(s.width/2);
			ry = spn::Rcp22Bit(s.height/2);
			if(_coordType == Coord::Window) {
				ox = -1.f;
				oy = 1.f;
				diry = -1.f;
			} else {
				ox = oy = 0.f;
				diry = 1.f;
			}
			auto& ofs = _offset;
			return spn::Mat33(rx*_scale.x,		0,						0,
							0,					ry*_scale.y,			0,
							ox + ofs.x*rx,		oy + ofs.y*ry*diry,		1);
		}
		void TextHUD::setWindowOffset(const spn::Vec2& ofs) {
			_coordType = Coord::Window;
			_offset = ofs;
		}
		void TextHUD::setScreenOffset(const spn::Vec2& ofs) {
			_coordType = Coord::Screen;
			_offset = ofs;
		}
		void TextHUD::setScale(const spn::Vec2& s) {
			_scale = s;
		}
		void TextHUD::setDepth(float d) {
			_depth = d;
		}
		void TextHUD::draw(GLEffect& e) const {
			// Zが0.0未満や1.0以上だと描画されないので、それより少し狭い範囲でクリップする
			float d = spn::Saturate(_depth, 0.f, 1.f-1e-4f);
			Text::draw(e, [d, this](auto& e){
				e.setUniform(unif2d::Depth, d);
				e.setUniform(U_Text, _makeMatrix(), true);
			});
		}
	}
}
