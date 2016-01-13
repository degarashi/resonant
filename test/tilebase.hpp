#pragma once
#include "displacement.hpp"

extern const rs::IdValue U_Repeat,
						U_Scale,
						U_Offset;
class TileFieldBase : public rs::DrawableObjT<TileFieldBase> {
	private:
		struct St_Default;
	protected:
		Displacement::IndexBuffV	_index;
		Displacement::HeightL	_heightL;
		spn::Vec3	_center,
					_scale;
		float		_width,			// 1タイル辺りのサイズ
					_repeat;
		int			_tileWidth,		// タイル列幅
					_nLevel;
		rs::HLTex	_hlTex;
		/*!
			\param[in]	rd		乱数生成器
			\param[in]	n		全体のタイル分割数
			\param[in]	vn		1つのタイルサイズ
			\param[in]	scale	マップ全体のサイズ
			\param[in]	height	マップ最大高
			\param[in]	height_att	マップディスプレースメント減衰係数
		*/
		TileFieldBase(spn::MTRandom& rd, spn::PowInt n, spn::PowInt vn, float scale, float height, float height_att, float th, float mv);
		void _prepareValues(rs::IEffect& e) const;
	public:
		void setViewPos(const spn::Vec3& p);
		void setTexture(rs::HTex hTex);
		void setTextureRepeat(float r);
};
DEF_LUAIMPORT(TileFieldBase)
