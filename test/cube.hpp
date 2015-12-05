#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "spinner/pose.hpp"

namespace rs {
	struct DrawTag;
}
class Engine;
class Cube : public spn::Pose3D {
	public:
		struct Type {
			enum E {
				Cone,
				Cube,
				Sphere,
				Torus,
				Capsule,
				Num
			};
		};
	private:
		static rs::WVb s_wVb[Type::Num][2];
		static rs::WIb s_wIb[Type::Num][2];
		rs::HLVb		_hlVb;
		rs::HLIb		_hlIb;
		rs::HLTex		_hlTex;
		void _initVb(Type::E typ, bool bFlat, bool bFlip);
	public:
		const static rs::IdValue	T_Cube,
									T_CubeDepth,
									T_CubeCubeDepth,
									T_CubeCube;
		/*! \param s[in]		一辺のサイズ
			\param hTex[in]		張り付けるテクスチャ
			\param bFlip[in]	面反転フラグ */
		Cube(float s, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip);
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
		CubeObj(float size, rs::HTex hTex, Type::E typ, bool bFlat, bool bFlip);
};
DEF_LUAIMPORT(CubeObj)
