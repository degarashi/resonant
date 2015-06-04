#pragma once
#include "../handle.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "../spinner/structure/profiler.hpp"

class ProfileShow : public rs::DrawableObjT<ProfileShow>,
					public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::CCoreID				_charId;
		mutable rs::HLText		_hlText;
		spn::Profiler::BlockSP	_spProfile;

		struct St_Default;
		void initState() override;
	public:
		ProfileShow(rs::CCoreID cid);
		rs::Priority getPriority() const override;
};
