#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "../differential.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "../util/textdraw.hpp"

extern const rs::GMessageId MSG_Visible;
extern const std::string g_fontName;
class InfoShow : public rs::DrawableObjT<InfoShow>,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::HDGroup			_hDg;
		rs::util::TextHUD	_textHud;
		std::u32string		_infotext;
		rs::diff::Effect	_count;
		spn::Vec2			_offset;

		struct MySt;
	public:
		static const rs::IdValue	T_Info;
		InfoShow(rs::HDGroup hDg, rs::Priority dprio);
		void setOffset(const spn::Vec2& ofs);
		rs::Priority getPriority() const override;
};
DEF_LUAIMPORT(InfoShow)
