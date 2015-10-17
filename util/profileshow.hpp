#pragma once
#include "../updater.hpp"
#include "textdraw.hpp"
#include "spinner/structure/profiler.hpp"

namespace rs {
	namespace util {
		class ProfileShow : public DrawableObjT<ProfileShow> {
			private:
				IdValue					_idText,
										_idRect;
				HDGroup					_hDg;
				TextHUD					_textHud;
				spn::Profiler::BlockSP	_spProfile;
				Priority				_uprio;
				spn::Vec2				_offset;

				struct St_Default;
			public:
				ProfileShow(IdValue idText, IdValue idRect, CCoreID cid, HDGroup hDg, Priority uprio, Priority dprio);
				void setOffset(const spn::Vec2& ofs);
				Priority getPriority() const override;
		};
	}
}
