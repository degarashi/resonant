#include "textdraw.hpp"
#include "../drawtag.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

namespace rs {
	namespace util {
		// ---------------------- Text ----------------------
		CCoreID_OP Text::cs_defaultCid;
		CCoreID Text::_GetDefaultCID() {
			if(!cs_defaultCid) {
				cs_defaultCid = mgr_text.makeCoreID("IPAGothic", rs::CCoreID(0, 20, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel));
			}
			return *cs_defaultCid;
		}
		Text::Text():
			_color(1),
			_bRefl(true)
		{
			_charId = _GetDefaultCID();
		}
		void Text::setCCoreId(CCoreID cid) {
			_charId = cid;
		}
		void Text::setText(spn::To32Str str) {
			_text = str.moveTo();
			_bRefl = true;
		}
		void Text::setText(HText h) {
			_hlText = h;
			_bRefl = false;
		}
		ColorA& Text::refColor() {
			return _color;
		}
		CCoreID Text::getCCoreId() const {
			return _charId;
		}
		HText Text::getText() const {
			if(_bRefl) {
				_bRefl = false;
				_hlText = mgr_text.createText(_charId, _text);
			}
			return _hlText;
		}
		void Text::draw(IEffect& e, const CBPreDraw& cbPre) const {
			getText();

			cbPre(e);
			e.setUniform(unif::Color, _color);
			_hlText->draw(e);
		}
		void Text::exportDrawTag(DrawTag& d) const {
			getText()->exportDrawTag(d);
		}
	}
}
