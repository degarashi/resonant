#pragma once
#include "tilebase.hpp"
#include "diffusion_u.hpp"

class STile {
	private:
		rs::HLVb		_vertex[2];
	public:
		STile(const Displacement::HeightL& h, int ox, int oy, spn::PowInt size, int stride);
		void draw(rs::IEffect& e, rs::HIb idx, const spn::Vec2& distRange) const;
};
class STileField : public rs::ObjectT<STileField, TileFieldBase>, public RayleighMie {
	private:
		using base_t = rs::ObjectT<STileField, TileFieldBase>;
		using STile_OP = spn::Optional<STile>;
		using STileV = std::vector<STile_OP>;
		STileV				_tile;
		Displacement::HLIbV	_indexArray;
		rs::HLVb			_vbSphere;
		rs::HLIb			_ibSphere;
		float				_viewMin,
							_viewCoeff;
		struct St_Default;

		std::pair<int,spn::Vec2> _calcLevel(float x, float y) const;
	public:
		STileField(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
							float scale, float height, float height_att, float th, float mv);
		void setViewDistanceCoeff(float dMin, float dCoeff);
};
DEF_LUAIMPORT(STileField)
