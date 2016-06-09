#include "textdraw.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"
#include "../systeminfo.hpp"

namespace rs {
	namespace util {
		// ---------------------- TextHUD ----------------------
		const IdValue TextHUD::U_Text = IEffect::GlxId::GenUnifId("mText");
		TextHUD::TextHUD():
			_coordType(Coord::Window),
			_offset(0),
			_scale(1),
			_depth(1)
		{}
		spn::Mat33 TextHUD::_makeMatrix() const {
			float rx, ry,
				  ox, oy,
				  diry;
			auto s = mgr_info.getScreenSize();
			rx = spn::Rcp22Bit(s.width/2);
			ry = spn::Rcp22Bit(s.height/2);
			auto ofs = _offset;
			if(_coordType == Coord::Window) {
				ox = -1.f;
				oy = 1.f;
				diry = -1.f;
				ofs.x *= rx;
				ofs.y *= ry;
			} else {
				ox = oy = 0.f;
				diry = 1.f;
			}
			return spn::Mat33(rx*_scale.x,		0,						0,
							0,					ry*_scale.y,			0,
							ox + ofs.x,			oy + ofs.y*diry,		1);
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
		int TextHUD::draw(IEffect& e) const {
			// Zが0.0未満や1.0以上だと描画されないので、それより少し狭い範囲でクリップする
			const float d = spn::Saturate(_depth, 0.f, 1.f-1e-4f);
			return Text::draw(e, [d, this](auto& e){
				e.setUniform(unif2d::Depth, d);
				e.setUniform(U_Text, _makeMatrix(), true);
			});
		}
	}
}
