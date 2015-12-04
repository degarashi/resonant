#include "geometry.hpp"

namespace {
	template <class I>
	auto MakeAdd3(I* dst) {
		return [dst](int i0, int i1, int i2) mutable {
			*dst++ = i0;
			*dst++ = i1;
			*dst++ = i2;
		};
	}
	template <class I>
	auto MakeAdd4(I* dst) {
		auto add3 = MakeAdd3(dst);
		return [add3](int i0, int i1, int i2, int i3) mutable {
			add3(i0, i1, i2);
			add3(i2, i3, i0);
		};
	}
}
namespace boom {
	namespace geo3d {
		void Geometry::UVUnwrapCylinder(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos)
		{
			uv.resize(srcPos.size());
			auto* pUv = uv.data();
			auto m = pose.getToLocal();
			for(auto& p : srcPos) {
				auto pl = p.asVec4(1) * m;
				Vec2 angDir(pl.x, pl.z);
				if(angDir.normalize() < 1e-5f)
					pUv->x = 0;
				else
					pUv->x = (spn::AngleValue(angDir) / spn::RadF::OneRotationAng).get();
				pUv->y = spn::Saturate(pl.y, 0.f, 1.f);
				++pUv;
			}
			Assert(Trap, pUv==uv.data()+uv.size())
		}
		void Geometry::MakeVertexNormalFlat(Vec3V& dstPos,
											IndexV& dstIndex,
											Vec3V& dstNormal,
											Vec2V& dstUv,
											const Vec3V& srcPos,
											const IndexV& srcIndex,
											const Vec2V& srcUv)
		{
			auto fnReserve = [](auto& v, auto size) {
				v.clear();
				v.reserve(size);
			};
			fnReserve(dstPos, srcPos.size());
			fnReserve(dstIndex, srcIndex.size());
			fnReserve(dstNormal, srcPos.size());
			fnReserve(dstUv, srcPos.size());

			struct Tmp {
				Vec3	pos,
						normal;
				Vec2	uv;

				bool operator == (const Tmp& t) const {
					return pos == t.pos &&
							normal == t.normal &&
							uv == t.uv;
				}
				size_t operator ()(const Tmp& t) const {
					return std::hash<Vec3>()(t.pos) ^
							std::hash<Vec3>()(t.normal) ^
							std::hash<Vec2>()(t.uv);
				}
			};
			using TmpV = std::vector<Tmp>;
			TmpV tmpv;
			using VMap = std::unordered_map<Tmp, int, Tmp>;
			VMap vmap;

			const int nI = srcIndex.size();
			AssertT(Trap, nI%3==0, std::invalid_argument, "Invalid face count")
			for(int i=0 ; i<nI ; i+=3) {
				const Vec3* pt[3] = {
					&srcPos[srcIndex[i]],
					&srcPos[srcIndex[i+1]],
					&srcPos[srcIndex[i+2]]
				};
				auto plane = spn::Plane::FromPts(*pt[0], *pt[1], *pt[2]);
				auto normal = plane.getNormal();
				// 座標とUVが同一なら同じ頂点とみなす
				for(int j=0 ; j<3 ; j++) {
					Tmp tmp{*pt[j], normal, srcUv[srcIndex[j]]};
					auto itr = vmap.find(tmp);
					if(itr == vmap.end()) {
						int idx = dstPos.size();
						itr = vmap.emplace(tmp, idx).first;
						dstPos.emplace_back(tmp.pos);
						dstNormal.emplace_back(tmp.normal);
						dstUv.emplace_back(tmp.uv);
					}
					dstIndex.emplace_back(itr->second);
				}
			}
		}
		void Geometry::MakeCube(Vec3V& dstPos,
								IndexV& dstIndex)
		{
			const int nV = 8;
			{
				// ---- 頂点座標 ----
				dstPos.resize(nV);
				auto* pV = dstPos.data();
				for(int i=0 ; i<nV ; i++) {
					*pV++ = Vec3((i&1)*2-1,
							((i>>1)&1)*2-1,
							((i>>2)&1)*2-1);
				}
				Assert(Trap, pV==dstPos.data()+nV)
			}
			{
				// ---- インデックス ----
				const int nI = 2*3*6;
				dstIndex.resize(nI);
				auto add4 = MakeAdd4(dstIndex.data());
				// front
				add4(0,2,3,1);
				// left
				add4(4,6,2,0);
				// right
				add4(1,3,7,5);
				// back
				add4(5,7,6,4);
				// top
				add4(2,6,7,3);
				// bottom
				add4(0,1,5,4);
			}
		}
	}
}
