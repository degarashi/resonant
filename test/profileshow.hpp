#pragma once
#include "../handle.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "spinner/structure/profiler.hpp"
#include "../util/textdraw.hpp"
#include "../util/screenrect.hpp"

class ProfileShow : public rs::DrawableObjT<ProfileShow>,
					public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::HDGroup				_hDg;
		rs::util::TextHUD		_textHud;
		spn::Profiler::BlockSP	_spProfile;
		spn::Vec2				_offset;

		struct St_Default;
	public:
		ProfileShow(rs::CCoreID cid, rs::HDGroup hDg);
		void setOffset(const spn::Vec2& ofs);
		rs::Priority getPriority() const override;
};
