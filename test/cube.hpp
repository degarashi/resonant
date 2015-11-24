#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "spinner/pose.hpp"

namespace rs {
	struct DrawTag;
}
class Engine;
class Cube : public spn::Pose3D {
	private:
		static rs::WVb s_wVb[2];
		rs::HLVb		_hlVb;
		rs::HLTex		_hlTex;
		spn::Vec3		_vLitPos;
		void _initVb(bool bFlip);
	public:
		const static rs::IdValue	T_Cube,
									U_litpos;
		/*! \param s[in]		一辺のサイズ
			\param hTex[in]		張り付けるテクスチャ
			\param bFlip[in]	面反転フラグ */
		Cube(float s, rs::HTex hTex, bool bFlip);
		void draw(Engine& e) const;
		void exportDrawTag(rs::DrawTag& d) const;
		void advance();
		void setLightPosition(const spn::Vec3& pos);
};
#include "../util/dwrapper.hpp"
class CubeObj : public rs::util::DWrapper<Cube> {
	public:
		CubeObj(rs::HDGroup hDg, float size, rs::HTex hTex, bool bFlip);
};
DEF_LUAIMPORT(CubeObj)
