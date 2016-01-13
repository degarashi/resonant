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
	static rs::HLVb MakeTileVertex0(spn::PowInt size);
	using Vec3V = std::vector<spn::Vec3>;
	static Vec3V MakeTileVertexNormal(const HeightL& h, int ox, int oy, spn::PowInt size, int stride);
};
