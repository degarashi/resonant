#pragma once
#include "test/tweak/node.hpp"
#include "../../util/textdraw.hpp"
#include "../../util/screenrect.hpp"

namespace tweak {
	class Drawer {
		private:
			rs::util::TextHUD		_text;			//!< 矩形描画クラス
			rs::util::WindowRect	_rect;			//!< テキスト描画クラス
			rs::IEffect&			_effect;		//!< 描画インタフェース
		public:
			/*!
				\param[in] e			描画インタフェース
			*/
			Drawer(rs::IEffect& e);
			//! 任意の色で矩形描画
			/*!
				\param rect			描画対象の矩形
				\param bWireframe	trueならワイヤフレームで描画
				\param color		矩形色
			*/
			void drawRect(const spn::RectF& rect, bool bWireframe, Color color);
			//! 塗りつぶし矩形とワイヤー矩形の両方描画
			void drawRectBoth(const spn::RectF& rect, Color color);
			int drawText(const Vec2& offset, rs::HText hText, Color color);
			//! 縦位置中央に補正したテキスト描画
			int drawTextVCenter(const spn::RectF& rect, rs::HText hText, Color color);
			int drawTexts(const Vec2&, Color) { return 0; }
			template <class T, class S, class... Ts>
			int drawTexts(const Vec2& offset, Color color, T&& t, S s, Ts&&... ts) {
				const int w = drawText(offset, std::forward<T>(t), color);
				return w + s +
						drawTexts(
							offset + Vec2(w+s, 0), color,
							std::forward<Ts>(ts)...
						);
			}
	};
}
