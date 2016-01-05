#pragma once
#include "../updater.hpp"
#include "../util//screenrect.hpp"

// 複数のテクスチャをオフセット値を含めて重ねがけ
class OffsetAdd : public rs::DrawableObjT<OffsetAdd> {
	private:
		constexpr static int		NTex = 3;
		static const rs::IdValue T_Offset[NTex],
								U_AddTex,
								U_Ratio,
								U_Offset;
		rs::HLTex				_hlAdd[NTex];
		mutable rs::HLFb		_hlFb;
		rs::util::ScreenRect	_rect;
		int						_nTex;
		float					_alpha,
								_offset,
								_ratio;

	public:
		OffsetAdd(rs::Priority p);
		void setAlpha(float a);
		void setOffset(float ofs);
		void setRatio(float r);
		void setAdd1(rs::HTex h0);
		void setAdd2(rs::HTex h0, rs::HTex h1);
		void setAdd3(rs::HTex h0, rs::HTex h1, rs::HTex h2);
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(OffsetAdd)
