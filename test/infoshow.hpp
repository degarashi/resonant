#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "../differential.hpp"
#include "../updater.hpp"
#include "../font.hpp"
#include "../util/textdraw.hpp"
#include "../util/screenrect.hpp"

extern const std::string g_fontName;
class TextShow : public rs::DrawableObjT<TextShow> {
	private:
		rs::util::TextHUD		_textHud;
		rs::util::WindowRect	_rect;		//!< テキスト背景

		struct St_Default;
	public:
		static const rs::IdValue	T_Text;
		TextShow(rs::Priority dprio);
		void setOffset(const spn::Vec2& ofs);
		void setText(const std::string& str);
		void setBGDepth(float d);
		void setBGColor(const spn::Vec3& c);
		void setBGAlpha(float a);
		rs::Priority getPriority() const override;
};
DEF_LUAIMPORT(TextShow)

class InfoShow : public rs::ObjectT<InfoShow, TextShow> {
	private:
		rs::HDGroup			_hDg;
		std::string			_baseText;
		rs::diff::Effect	_count;
		std::string			_stateName;

		struct St_Default;
	public:
		InfoShow(rs::HDGroup hDg, rs::Priority dprio);
};
DEF_LUAIMPORT(InfoShow)
