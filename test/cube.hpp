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
		void _initVb(bool bFlip);
	public:
		const static rs::IdValue	T_Cube,
									T_CubeDepth;
		/*! \param s[in]		一辺のサイズ
			\param hTex[in]		張り付けるテクスチャ
			\param bFlip[in]	面反転フラグ */
		Cube(float s, rs::HTex hTex, bool bFlip);
		void draw(Engine& e) const;
		void exportDrawTag(rs::DrawTag& d) const;
		void advance();
};
#include "../util/dwrapper.hpp"
class CubeObj : public rs::DrawableObjT<CubeObj>,
				public Cube
{
	private:
		struct St_Default;
	public:
		CubeObj(float size, rs::HTex hTex, bool bFlip);
};
DEF_LUAIMPORT(CubeObj)
