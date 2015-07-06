#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "../differential.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "../util/textdraw.hpp"

extern const rs::GMessageId MSG_Visible;
class InfoShow : public rs::DrawableObjT<InfoShow>,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::util::TextHUD	_textHud;
		std::u32string		_infotext;
		rs::diff::Effect	_count;

		struct MySt;
	public:
		static const rs::IdValue	T_Info;
		InfoShow();
		rs::Priority getPriority() const override;
};
