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
			spn::RectF				_range;			//!< 仮想空間上での表示範囲
			const INode* const		_cursor;		//!< カーソル位置にある要素
			Vec2					_offset,		//!< 描画オフセット
									_cursorAt;		//!< カーソル描画オフセット
		public:
			/*!
				\param[in] range		仮想空間上での表示範囲
				\param[in] offset		実際の描画オフセット(左上からの)
				\param[in] cur			カーソル位置にある要素
				\param[in] e			描画インタフェース
			*/
			Drawer(const spn::RectF& range, const Vec2& offset,
					const INode::SP& cur, rs::IEffect& e);
			//! 描画処理中に出現したカーソル項目の位置
			const Vec2& getCursorAt() const;
			//! 描画の範囲内かの判定
			/*!
				\param p カーソル位置記憶用
				\param r 描画判定対象の矩形
				\return 描画の必要があるならtrue
			*/
			bool checkDraw(const INode* p, const spn::RectF& r);
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
