#pragma once
#include "../font.hpp"
#include "../glx_id.hpp"
#include "spinner/optional.hpp"
#include "spinner/pose.hpp"
#include "color.hpp"

namespace rs {
	struct DrawTag;
	struct IEffect;
	class SystemUniform2D;
	class SystemUniform3D;
	namespace util {
		enum class Align {
			Negative,
			Positive,
			Middle
		};
		using Size_OP = spn::Optional<spn::Size>;
		using CCoreID_OP = spn::Optional<CCoreID>;
		class Text {
			private:
				static CCoreID_OP	cs_defaultCid;
				std::u32string		_text;
				CCoreID				_charId;
				ColorA				_color;
				mutable bool		_bRefl;
				mutable HLText		_hlText;

				static CCoreID _GetDefaultCID();
				using CBPreDraw = std::function<void (IEffect&)>;
			public:
				Text();
				void setCCoreId(CCoreID cid);
				HText getText() const;
				CCoreID getCCoreId() const;
				void setText(spn::To32Str str);
				void setText(HText h);
				ColorA& refColor();
				int draw(IEffect& e, const CBPreDraw& cbPre=[](auto&){}) const;
				void exportDrawTag(DrawTag& d) const;
		};
		//! テキスト描画クラス (for HUD)
		class TextHUD : public Text {
			private:
				enum class Coord {
					Window,		//!< ウィンドウ座標系 (左上を原点としたX+ Y-のピクセル単位)
					Screen		//!< スクリーン座標系 (中央を原点としたX[-Asp,Asp], Y[-1,1])
				};
				Coord				_coordType;
				spn::Vec2			_offset,
									_scale;
				float				_depth;

				spn::Mat33 _makeMatrix() const;
			public:
				TextHUD();
				const static IdValue U_Text;
				//! ウィンドウ座標系で範囲指定
				void setWindowOffset(const spn::Vec2& ofs);
				//! スクリーン座標系で範囲指定
				void setScreenOffset(const spn::Vec2& ofs);
				void setScale(const spn::Vec2& s);
				void setDepth(float d);
				int draw(IEffect& e) const;
		};
		// 1行の縦をY=1としたサイズに内部変換
		// H,V {Negative, Positive, Middle}
		//! テキスト描画クラス (for 2D)
		class Text2D : public spn::Pose2D, public Text {
			private:
				float		_lineHeight,
							_depth;
			public:
				Text2D(float lh);
				void setLineHeight(float lh);
				void setDepth(float d);
				int draw(IEffect& e, bool bRefresh=false) const;
		};
		//! テキスト描画クラス (for 3D sprite)
		class Text3D : public spn::Pose3D, public Text {
			private:
				float	_lineHeight;
				bool	_bBillboard;		//!< trueなら描画時にビルボード変換
			public:
				Text3D(float lh, bool bBillboard);
				void setLineHeight(float lh);
				void setBillboard(bool b);
				int draw(IEffect& e, bool bRefresh=false) const;
		};
	}
}
