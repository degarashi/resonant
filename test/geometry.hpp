#pragma once
#include "boomstick/geom2D.hpp"
#include "boomstick/geom3D.hpp"

namespace boom {
	using Vec2V = std::vector<spn::Vec2>;
	using Vec3V = std::vector<spn::Vec3>;
	using Vec4V = std::vector<spn::Vec4>;
	using IndexV = std::vector<uint16_t>;
	template <class Itr, class OItr, class Ofs>
	void FlipFace(Itr itr, Itr itrE, OItr oitr, Ofs offset) {
		AssertT(Trap, (itrE-itr)%3==0, std::invalid_argument, "Invalid face count")
		while(itr < itrE) {
			auto v0 = *itr++;
			auto v1 = *itr++;
			auto v2 = *itr++;
			*oitr++ = v0 + offset;
			*oitr++ = v2 + offset;
			*oitr++ = v1 + offset;
		}
	}
	float Linear(float value, float minv, float maxv);
	float Bounce(float value, float freq);
	namespace geo3d {
		struct Geometry {
			static void UVUnwrapCylinder(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos);
			static void UVUnwrapSphere(Vec2V& uv,
									float nFreqU,
									float nFreqV,
									const Pose3D& pose,
									const Vec3V& srcPos);
			static void UVUnwrapPlane(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos);
			static void UVUnwrapCube(Vec2V& dstUv,
									const Pose3D& pose,
									const Vec3V& srcPos,
									const IndexV& srcIndex);
			static void MakeVertexNormal(Vec3V& dstNormal,
											const Vec3V& srcPos,
											const IndexV& srcIndex);
			static void MakeVertexNormalFlat(Vec3V& dstPos,
											IndexV& dstIndex,
											Vec3V& dstNormal,
											Vec2V& dstUv,
											const Vec3V& srcPos,
											const IndexV& srcIndex,
											const Vec2V& srcUv);
			// Sphere
			static void MakeSphere(Vec3V& dstPos,
									IndexV& dstIndex,
									int divH,
									int divV);
			// Arc
			static void MakeArc(Vec3V& dstPos,
								IndexV& dstIndex,
								spn::RadF angle,
								int divH,
								int divV,
								bool bCap);
			// Cube
			static void MakeCube(Vec3V& dstPos,
								IndexV& dstIndex);
			// Capsule
			static void MakeCapsule(Vec3V& dstPos,
									IndexV& dstIndex,
									float length,
									int div);
			// Cone
			static void MakeCone(Vec3V& dstPos,
								IndexV& dstIndex,
								int div);
			// Torus
			static void MakeTorus(Vec3V& dstPos,
								IndexV& dstIndex,
								float radius,
								int divH,
								int divR);
			// for bumpmap
			static void CalcTangent(Vec4V& dstTan,
									Vec3V& srcPos,
									IndexV& srcIndex,
									Vec3V& srcNormal,
									Vec2V& srcUv);
		};
	}
	namespace geo2d {
		struct Geometry {
			// Rect
			static void MakeRect(Vec2V& dstPos,
								IndexV& dstIndex);
			// Circle
			static void MakeCircle(Vec2V& dstPos,
									IndexV& dstIndex,
									int div);
			// Capsule
			static void MakeCapsule(Vec2V& dstPos,
									IndexV& dstIndex,
									int div);
			static void MakeArc(Vec2V& dstPos,
								IndexV& dstIndex,
								spn::RadF angle,
								int div,
								bool bCap);
		};
	}
}
