#include "geometry.hpp"

namespace {
	template <class I>
	auto MakeAdd3(I*& dst) {
		return [&dst](int i0, int i1, int i2) mutable {
			*dst++ = i0;
			*dst++ = i1;
			*dst++ = i2;
		};
	}
	template <class I>
	auto MakeAdd4(I*& dst) {
		auto add3 = MakeAdd3(dst);
		return [add3](int i0, int i1, int i2, int i3) mutable {
			add3(i0, i1, i2);
			add3(i2, i3, i0);
		};
	}
}
namespace boom {
	float Linear(const float value, const float minv, const float maxv) {
		return (value - minv) / (maxv - minv);
	}
	float Bounce(float value, float freq) {
		AssertP(Trap, freq>=1 && spn::IsInRange(value, 0.f, 1.f))
		freq = 1/freq;
		value = std::fmod(value, freq);
		if(value > freq/2)
			return 1.f - Linear(value-freq/2, 0.f, freq/2);
		return Linear(value, 0.f, freq/2);
	}
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
		void Geometry::UVUnwrapSphere(Vec2V& uv,
								const float nFreqU,
								const float nFreqV,
								const Pose3D& pose,
								const Vec3V& srcPos)
		{
			uv.resize(srcPos.size());
			auto* pUv = uv.data();
			const auto mLocal = pose.getToLocal();
			for(auto& p : srcPos) {
				auto pl = p.asVec4(1) * mLocal;
				Vec2 angDirU(pl.x, pl.y);
				const auto bU = angDirU.normalize() < 1e-5f;
				if(bU)
					pUv->x = 0;
				else
					pUv->x = (spn::AngleValue(angDirU) / spn::RadF::OneRotationAng).get();
				if(std::abs(pl.z) < 1e-5f)
					pUv->y = 0.5f;
				else {
					if(bU)
						pUv->y = (pl.z >= 0) ? 1.f : 0.f;
					else {
						pl.normalize();
						float d = spn::Saturate(pl.dot(Vec3(angDirU.x, angDirU.y, 0)), 1.f);
						d = std::acos(d);
						if(pl.z < 0)
							d *= -1;
						pUv->y = (d / spn::PI) + 0.5f;
					}
				}
				if(nFreqU > 0)
					pUv->x = Bounce(pUv->x, nFreqU);
				if(nFreqV > 0)
					pUv->y = Bounce(pUv->y, nFreqV);
				++pUv;
			}
			Assert(Trap, pUv==uv.data()+uv.size())
		}
		void Geometry::UVUnwrapPlane(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos)
		{
			auto toLocal = pose.getToLocal();
			uv.resize(srcPos.size());
			auto* pUv = uv.data();
			for(auto& v : srcPos)
				*pUv++ = (v.asVec4(1) * toLocal).asVec2();
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
					Tmp tmp{*pt[j], normal, srcUv[srcIndex[i+j]]};
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
		void Geometry::MakeVertexNormal(Vec3V& dstNormal,
								const Vec3V& srcPos,
								const IndexV& srcIndex)
		{
			const int nI = srcIndex.size(),
					nV = srcPos.size();
			AssertT(Trap, nI%3==0, std::invalid_argument, "Invalid face count")
			dstNormal.resize(nV);

			struct Tmp {
				Vec3	vec{0,0,0};
				int		count=0;
				void add(const Vec3& v) {
					vec += v;
					++count;
				}
				Vec3 get() const {
					return vec.normalization();
				}
			};
			std::vector<Tmp> vn(nV);
			for(int i=0 ; i<nI ; i+=3) {
				// 面法線を計算
				auto i0 = srcIndex[i],
					i1 = srcIndex[i+1],
					i2 = srcIndex[i+2];
				auto p = Plane::FromPts(srcPos[i0],
										srcPos[i1],
										srcPos[i2]);
				auto nml = p.getNormal();
				vn[i0].add(nml);
				vn[i1].add(nml);
				vn[i2].add(nml);
			}
			// 頂点法線を計算
			for(int i=0 ; i<nV ; i++) {
				dstNormal[i] = vn[i].get();
			}
		}
		void Geometry::MakeSphere(Vec3V& dstPos,
								IndexV& dstIndex,
								const int divH,
								const int divV)
		{
			MakeArc(dstPos, dstIndex, spn::RadF((spn::PI*2 / divH) * (divH-1)), divH-1, divV, true);
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
				auto* pI = dstIndex.data();
				auto add4 = MakeAdd4(pI);
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
				Assert(Trap, pI==dstIndex.data()+nI)
			}
		}
		void Geometry::MakeArc(Vec3V& dstPos,
								IndexV& dstIndex,
								const spn::RadF angle,
								const int divH,
								const int divV,
								const bool bCap)
		{
			AssertT(Trap, divH>=2 && divV>=2, std::invalid_argument, "Invalid division number")
			const int nV = 2 + (divV-1) * (divH+1);
			{
				dstPos.resize(nV);
				auto* pV = dstPos.data();
				*pV++ = Vec3(0, 0, -1);
				for(int i=0 ; i<=divH ; i++) {
					const float diffV = spn::PI / divV;
					float curV = 0;
					auto q = Quat::RotationZ(-angle * i / divH);
					for(int j=1 ; j<divV ; j++) {
						curV += diffV;
						*pV++ = Vec3(0, std::sin(curV), -std::cos(curV)) * q;
					}
				}
				*pV++ = Vec3(0, 0, 1);
				Assert(Trap, pV==dstPos.data()+nV)
			}
			{
				const int nI = divH*2*3 + (divV-2)*6*divH
								+ ((bCap) ? ((divV-2)*6 + 2*3) : 0);
				dstIndex.resize(nI);
				auto* pI = dstIndex.data();
				auto add3 = MakeAdd3(pI);
				auto add4 = MakeAdd4(pI);
				const int base = 1,
						stride = divV-1,
						baseE = base + stride - 1;
				for(int i=0 ; i<divH ; i++) {
					const int ofs0 = i*stride,
							ofs1 = ofs0+stride;
					// 始点側
					add3(0, base+ofs0, base+ofs1);
					// 中央部
					for(int j=0 ; j<divV-2 ; j++)
						add4(base+ofs0+j, base+ofs0+j+1, base+ofs1+j+1, base+ofs1+j);
					// 終点側
					add3(nV-1, baseE+ofs1, baseE+ofs0);
				}
				// キャップ部
				if(bCap) {
					// 始点側
					add3(0, nV-1-stride, base);
					// 中央部
					for(int i=0 ; i<divV-2 ; i++) {
						const int base0 = base+i,
								base1 = nV-1-stride+i;
						add4(base0+1, base0, base1, base1+1);
					}
					// 終点側
					add3(nV-1, baseE, nV-2);
				}
				Assert(Trap, pI==dstIndex.data()+nI)
			}
		}
		void Geometry::MakeCapsule(Vec3V& dstPos,
									IndexV& dstIndex,
									const float length,
									const int div)
		{
			// 右半分
			MakeArc(dstPos, dstIndex, spn::RadF(spn::PI), div, div, false);
			const auto mRot = spn::Mat33::RotationY(spn::DegF(90));
			// Y軸を中心に90度回転
			for(auto& v : dstPos)
				v *= mRot;

			const int nV = dstPos.size();
			// 左半分
			dstPos.resize(nV*2);
			// 右半分の座標を反転しつつコピー
			for(int i=0 ; i<nV ; i++)
				dstPos[i+nV] = dstPos[i] * Vec3(1,1,-1) + Vec3(0,0,length);
			const int nI = dstIndex.size();
			dstIndex.resize(nI*2 + div*2*6);
			// 面は反転
			FlipFace(dstIndex.begin(), dstIndex.begin()+nI, dstIndex.begin()+nI, nV);

			// 中央部分
			const int roundN = 2+(div-1)*2;
			std::vector<int> idxF(roundN);
			auto* pR = idxF.data();
			*pR++ = 0;
			for(int i=0 ; i<div-1 ; i++)
				*pR++  = i+1;
			*pR++ = nV-1;
			for(int i=0 ; i<div-1 ; i++)
				*pR++ = nV-i-2;
			Assert(Trap, pR==idxF.data()+roundN)

			auto* pI = dstIndex.data()+nI*2;
			auto add4 = MakeAdd4(pI);
			for(int i=0 ; i<div*2-1 ; i++) {
				const int baseF0 = idxF[i],
							baseF1 = idxF[i+1],
							baseB0 = baseF0 + nV,
							baseB1 = baseF1 + nV;
				add4(baseF1, baseF0, baseB0, baseB1);
			}
			const int baseF0 = idxF.back(),
						baseF1 = idxF[0],
						baseB0 = baseF0 + nV,
						baseB1 = baseF1 + nV;
			add4(baseF1, baseF0, baseB0, baseB1);
			Assert(Trap, pI==dstIndex.data()+dstIndex.size())
		}
		void Geometry::MakeCone(Vec3V& dstPos,
								IndexV& dstIndex,
								const int div)
		{
			AssertT(Trap, div>=3, std::invalid_argument, "Invalid division number")
			const int nV = 2+div;
			{
				dstPos.resize(nV);
				auto* pV = dstPos.data();
				*pV++ = Vec3(0, 0, 0);
				const float diff = spn::PI*2 / div;
				float cur = 0;
				for(int i=0 ; i<div ; i++) {
					*pV++ = Vec3(std::sin(cur)*0.5f, std::cos(cur)*0.5f, 1);
					cur += diff;
				}
				*pV++ = Vec3(0, 0, 1);
				Assert(Trap, pV==dstPos.data()+nV)
			}
			{
				const int nI = (div+div)*3;
				dstIndex.resize(nI);
				auto* pI = dstIndex.data();
				auto add3 = MakeAdd3(pI);
				// 側面
				for(int i=0 ; i<div-1 ; i++)
					add3(0, i+1, i+2);
				add3(0, div, 1);
				// 底面
				for(int i=0 ; i<div-1 ; i++)
					add3(i+2, i+1, nV-1);
				add3(1, div, nV-1);
				Assert(Trap, pI==dstIndex.data()+nI)
			}
		}
		void Geometry::MakeTorus(Vec3V& dstPos,
								IndexV& dstIndex,
								const float radius,
								const int divH,
								const int divR)
		{
			AssertT(Trap, divH>=3 && divR>=3, std::invalid_argument, "Invalid division number")
			const int nV = divR * divH;
			{
				dstPos.resize(nV);
				auto* pV = dstPos.data();
				const float diffH = spn::PI*2 / divH;
				float curH = 0;
				for(int i=0 ; i<divH ; i++) {
					auto q = Quat::RotationZ(spn::RadF(curH));
					const float diffR = spn::PI*2 / divR;
					float curR = 0;
					for(int j=0 ; j<divR ; j++) {
						*pV = Vec3(1+std::sin(curR)*radius,
										0,
										-std::cos(curR)*radius);
						*pV *= q;
						++pV;
						curR += diffR;
					}
					curH += diffH;
				}
				Assert(Trap, pV==dstPos.data()+nV)
			}
			{
				const int nI = 6*divR*divH;
				dstIndex.resize(nI);
				auto* pI = dstIndex.data();
				auto add4 = MakeAdd4(pI);
				auto fnR = [&add4,divR](int base0, int base1) {
					for(int j=0 ; j<divR-1 ; j++)
						add4(base0+j, base1+j, base1+j+1, base0+j+1);
					add4(base0, base0+divR-1, base1+divR-1, base1);
				};
				for(int i=0 ; i<divH-1 ; i++) {
					const int base0 = i*divR,
								base1 = base0+divR;
					fnR(base0, base1);
				}
				fnR((divH-1)*divR, 0);
				Assert(Trap, pI==dstIndex.data()+nI)
			}
		}
	}
}
