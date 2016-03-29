#pragma once
#include "spinner/size.hpp"
#include "../handle.hpp"
#include "../updater.hpp"
#include "spinner/structure/offsetindex.hpp"

namespace spn {
	class MTRandom;
}
namespace boom {
	using IndexV = std::vector<uint16_t>;
}
struct Displacement {
	using HeightL = std::vector<float>;
	using IndexV = boom::IndexV;
	using IndexV2 = std::vector<IndexV>;
	using OIndex = spn::OffsetIndex<uint16_t>;
	using OIndexV = std::vector<OIndex>;
	//! 中点変位によるランダム地形作製
	/*!
		\param[in] rd			ランダム生成器
		\param[in] size			フィールドXYサイズ
		\param[in] n			1辺のフィールドタイル数(2の乗数)
		\param[in] height		最大高さ
		\param[in] height_att	高さ減衰係数
	*/
	static HeightL MakeDisplacement(spn::MTRandom& rd, spn::PowInt n, float height_att);
	static void Smooth(HeightL& h, spn::PowInt n, float th, float mv);
	enum Dip {
		Left,
		Top,
		Right,
		Bottom,
		Center,
		Num,
		End = Num
	};
	struct Tile {
		IndexV		full;
		// Index[SubLevel] L,T,R,B,Center
		OIndexV		dip;
	};
	using TileV = std::vector<Tile>;
	// 指定されたタイルサイズを元に頂点&インデックス出力
	static TileV MakeIndex(spn::PowInt w);

	struct DipBuff {
		rs::HLIb	ib;
		int			offset[Dip::Num+1];
	};
	using DipBuffV = std::vector<DipBuff>;
	struct IndexBuff {
		rs::HLIb	full;
		DipBuffV	dip;
	};
	using IndexBuffV = std::vector<IndexBuff>;

	static IndexBuffV MakeBuffer(const TileV& tile);
};
class Tile {
	private:
		rs::HLVb		_vertex[2];
	public:
		Tile(const Displacement::HeightL& h, int ox, int oy, spn::PowInt size, int stride);
		void draw(rs::IEffect& e, const Displacement::IndexBuffV& idxv, int center, int left, int top, int right, int bottom) const;
};
class TileField : public rs::DrawableObjT<TileField> {
	private:
		Displacement::IndexBuffV	_index;

		using Tile_OP = spn::Optional<Tile>;
		using TileV = std::vector<Tile_OP>;
		TileV		_tile;
		struct St_Default;

		spn::Vec3	_center,
					_scale;
		float		_dMin, _dMax,
					_width,			// 1タイル辺りのサイズ
					_repeat;
		int			_tileWidth,		// タイル列幅
					_nLevel;
		rs::HLTex	_hlTex;

	public:
		/*!
			\param[in]	rd		乱数生成器
			\param[in]	n		全体のタイル分割数
			\param[in]	vn		1つのタイルサイズ
			\param[in]	scale	マップ全体のサイズ
			\param[in]	height	マップ最大高
			\param[in]	height_att	マップディスプレースメント減衰係数
		*/
		TileField(spn::MTRandom& rd, spn::PowInt n, spn::PowInt vn, float scale, float height, float height_att, float th, float mv);
		void setViewPos(const spn::Vec3& p);
		void setViewDistanceCoeff(float dMin, float dMax);
		void setTexture(rs::HTex hTex);
		void setTextureRepeat(float r);
};
DEF_LUAIMPORT(TileField)
