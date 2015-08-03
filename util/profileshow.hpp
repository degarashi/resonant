#pragma once
#include "../updater.hpp"
#include "textdraw.hpp"
#include "spinner/structure/profiler.hpp"

namespace rs {
	namespace util {
		class ProfileShow : public DrawableObjT<ProfileShow>,
							public spn::EnableFromThis<HDObj>
		{
			private:
				IdValue					_idText,
										_idRect;
				HDGroup					_hDg;
				TextHUD					_textHud;
				spn::Profiler::BlockSP	_spProfile;
				spn::Vec2				_offset;

				struct St_Default;
			public:
				ProfileShow(IdValue idText, IdValue idRect, CCoreID cid, HDGroup hDg);
				void setOffset(const spn::Vec2& ofs);
				Priority getPriority() const override;
		};
	}
}
