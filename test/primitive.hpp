#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "spinner/pose.hpp"

namespace rs {
	struct DrawTag;
}
using Vec3V = std::vector<spn::Vec3>;
using Vec4V = std::vector<spn::Vec4>;
class Engine;
class Primitive : public spn::Pose3D {
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
		static rs::WVb s_wVbLine[Type::Num][2];
		static rs::WIb s_wIb[Type::Num][2];
		rs::HLVb		_hlVb,
						_hlVbLine;
		rs::HLIb		_hlIb;
		rs::HLTex		_hlTex,
						_hlTexNormal;
		bool			_bShowNormal;
		void _initVb(Type::E typ, bool bFlat, bool bFlip);
		static rs::HLVb _MakeVbLine(const Vec3V& srcPos, const Vec3V& srcNormal, const Vec4V& srcTangentC);
	public:
		const static rs::IdValue	T_Prim,
									T_PrimDepth,
									T_PrimCubeDepth,
									T_PrimCube,
									T_PrimLine,
									T_PrimDLDepth;
		/*! \param s[in]		一辺のサイズ
			\param hTex[in]		張り付けるテクスチャ
			\param hTexNormal[in] 張り付ける法線テクスチャ
			\param bFlip[in]	面反転フラグ */
		Primitive(float s, rs::HTex hTex, rs::HTex hTexNormal, Type::E typ, bool bFlat, bool bFlip);
		void draw(Engine& e) const;
		void exportDrawTag(rs::DrawTag& d) const;
		void advance(float t);
		void showNormals(bool b);
};
#include "../util/dwrapper.hpp"
class PrimitiveObj : public rs::DrawableObjT<PrimitiveObj>,
				public Primitive
{
	private:
		struct St_Default;
	public:
		PrimitiveObj(float size, rs::HTex hTex, rs::HTex hTexNormal, Type::E typ, bool bFlat, bool bFlip);
};
DEF_LUAIMPORT(PrimitiveObj)
