#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "../differential.hpp"
#include "../updater.hpp"
#include "../font.hpp"

extern const rs::GMessageId MSG_Visible;
class InfoShow : public rs::DrawableObjT<InfoShow>,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		std::u32string		_infotext;
		rs::CCoreID			_charId;
		mutable rs::HLText	_hlText;
		rs::diff::Effect	_count;

		struct MySt;
		void initState() override;
	public:
		static const rs::IdValue	T_Info,
									U_Text;
		InfoShow();
		rs::Priority getPriority() const override;
};
