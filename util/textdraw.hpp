#pragma once
#include "../font.hpp"
#include "../glx_id.hpp"
#include "spinner/optional.hpp"
#include "spinner/pose.hpp"

namespace rs {
	struct DrawTag;
	class GLEffect;
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
				IdValue				_idTech;
				std::u32string		_text;
				CCoreID				_charId;
				Size_OP				_textSize;
				float				_alpha;
				mutable rs::HLText	_hlText;
				void _makeTextCache() const;

				static CCoreID _GetDefaultCID();
				using CBPreDraw = std::function<void (GLEffect&)>;
			public:
				Text(IdValue idTech);
				void setCCoreId(CCoreID cid);
				HText getText() const;
				CCoreID getCCoreId() const;
				void setTextSize(Size_OP s=spn::none);
				void setText(spn::To32Str str);
				void setAlpha(float a);
				void draw(GLEffect& e, const CBPreDraw& cbPre=[](auto&){}) const;
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
				TextHUD(IdValue idTech);
				const static IdValue U_Text;
				//! ウィンドウ座標系で範囲指定
				void setWindowOffset(const spn::Vec2& ofs);
				//! スクリーン座標系で範囲指定
				void setScreenOffset(const spn::Vec2& ofs);
				void setScale(const spn::Vec2& s);
				void setDepth(float d);
				void draw(GLEffect& e) const;
		};
		// 1行の縦をY=1としたサイズに内部変換
		// H,V {Negative, Positive, Middle}
		//! テキスト描画クラス (for 2D)
		class Text2D : public spn::Pose2D, public Text {
			private:
				float		_lineHeight,
							_depth;
			public:
				Text2D(IdValue idTech, float lh);
				void setLineHeight(float lh);
				void setDepth(float d);

				template <class T>
				void draw(T& t) const { draw(t, t, false); }
				void draw(GLEffect& e, SystemUniform2D& su2d, bool bRefresh) const;
		};
		//! テキスト描画クラス (for 3D sprite)
		class Text3D : public spn::Pose3D, public Text {
			private:
				float	_lineHeight;
				bool	_bBillboard;		//!< trueなら描画時にビルボード変換
			public:
				Text3D(IdValue idTech, float lh, bool bBillboard);
				void setLineHeight(float lh);
				void setBillboard(bool b);

				template <class T>
				void draw(T& t) const { draw(t, t, false); }
				void draw(GLEffect& e, SystemUniform3D& su3d, bool bRefresh) const;
		};
	}
}
