#pragma once
#include "../handle.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "../spinner/structure/profiler.hpp"
#include "../util/textdraw.hpp"

class ProfileShow : public rs::DrawableObjT<ProfileShow>,
					public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::util::TextHUD		_textHud;
		spn::Profiler::BlockSP	_spProfile;

		struct St_Default;
	public:
		ProfileShow(rs::CCoreID cid);
		rs::Priority getPriority() const override;
};
