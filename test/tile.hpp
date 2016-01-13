#pragma once
#include "tilebase.hpp"

class Tile {
	private:
		rs::HLVb		_vertex[2];
	public:
		Tile(const Displacement::HeightL& h, int ox, int oy, spn::PowInt size, int stride);
		void draw(rs::IEffect& e, const Displacement::IndexBuffV& idxv, int center, int left, int top, int right, int bottom) const;
};
class TileField : public rs::ObjectT<TileField, TileFieldBase> {
	private:
		using base_t = rs::ObjectT<TileField, TileFieldBase>;
		using Tile_OP = spn::Optional<Tile>;
		using TileV = std::vector<Tile_OP>;
		TileV		_tile;
		float		_dMin, _dMax;
		struct St_Default;

		int _calcLevel(float x, float y) const;
	public:
		TileField(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
							float scale, float height, float height_att, float th, float mv);
		void setViewDistanceCoeff(float dMin, float dMax);
};
DEF_LUAIMPORT(TileField)
