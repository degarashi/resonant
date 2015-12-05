#pragma once
#include "boomstick/geom3D.hpp"

namespace boom {
	using Vec2V = std::vector<spn::Vec2>;
	using Vec3V = std::vector<spn::Vec3>;
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
	namespace geo3d {
		struct Geometry {
			static void UVUnwrapCylinder(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos);
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
		};
	}
}
