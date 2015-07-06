#include "textdraw.hpp"
#include "../drawtag.hpp"
#include "../glx.hpp"
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
		Text::Text(IdValue idTech):
			_idTech(idTech),
			_alpha(1.f)
		{
			_charId = _GetDefaultCID();
		}
		void Text::setCCoreId(CCoreID cid) {
			_charId = cid;
		}
		void Text::setTextSize(Size_OP s) {
			_textSize = s;
		}
		void Text::setText(spn::To32Str str) {
			_text = str.moveTo();
		}
		void Text::setAlpha(float a) {
			_alpha = a;
		}
		CCoreID Text::getCCoreId() const {
			return _charId;
		}
		void Text::draw(GLEffect& e, const CBPreDraw& cbPre) const {
			e.setTechPassId(_idTech);
			cbPre(e);
			e.setUniform(unif::Alpha, _alpha);
			_hlText = mgr_text.createText(_charId, _text);
			_hlText->draw(&e);
		}
		void Text::exportDrawTag(DrawTag& d) const {
			_hlText->exportDrawTag(d);
			d.idTechPass = _idTech;
		}
	}
}