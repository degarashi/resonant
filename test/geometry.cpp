#include "geometry.hpp"

namespace rs {
	enum class CubeFace {
		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ,
		Num
	};
}
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
	spn::RadF CalcAngle(const Vec3& p0, const Vec3& p1, const Vec3& p2) {
		return spn::RadF(std::acos(spn::Saturate((p1-p0).normalization().dot((p2-p0).normalization()), 1.f)));
	}
	namespace {
		template <int N>
		struct SumVec {
			using Vec = spn::VecT<N,false>;
			Vec		vec{0,0,0};
			int		count=0;
			void add(const Vec& v) {
				vec += v;
				++count;
			}
			Vec getNormal() const {
				return vec.normalization();
			}
			Vec getAverage() const {
				return vec / count;
			}
			Vec get() const {
				return vec;
			}
			int num() const {
				return count;
			}
		};
		template <int N>
		using SumVecV = std::vector<SumVec<N>>;
	}
	namespace geo3d {
		using spn::Mat23;
		void Geometry::UVUnwrapCylinder(Vec2V& uv,
									const Pose3D& pose,
									const Vec3V& srcPos)
		{
			uv.resize(srcPos.size());
			auto* pUv = uv.data();
			auto m = pose.getToLocal();
			for(auto& p : srcPos) {
				auto pl = p.asVec4(1) * m;
				Vec2 angDir(pl.x, pl.y);
				if(angDir.normalize() < 1e-5f)
					pUv->x = 0;
				else
					pUv->x = (spn::AngleValue(angDir) / spn::RadF::OneRotationAng).get();
				pUv->y = spn::Saturate(pl.z, 0.f, 1.f);
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
		namespace {
			spn::RadF Angle(const Vec2& dir) {
				const float d = spn::Saturate(dir.dot(Vec2(0,1)), 1.f),
							ac = std::acos(d);
				return spn::RadF((dir.x < 0) ? ac : -ac);
			}
			rs::CubeFace GetCubeFaceId(const Vec3& dir) {
				float best = dir.m[0];
				int bestId = 0;
				for(int j=1 ; j<6 ; j++) {
					float p = dir.m[j/2];
					if((j&1) == 1)
						p *= -1;
					if(p > best) {
						bestId = j;
						best = p;
					}
				}
				return static_cast<rs::CubeFace>(bestId);
			}
			using Func = Vec2 (*)(const Vec3&);
			const Func Anglefunc[] = {
				[](auto& v){ return Vec2(Angle({-v.z,v.x}).get(), Angle({v.y,v.x}).get()); },
				[](auto& v){ return Vec2(Angle({v.z,-v.x}).get(), Angle({v.y,-v.x}).get()); },

				[](auto& v){ return Vec2(Angle({v.x, v.y}).get(), Angle({-v.z,v.y}).get()); },
				[](auto& v){ return Vec2(Angle({v.x,-v.y}).get(), Angle({v.z,-v.y}).get()); },

				[](auto& v){ return Vec2(Angle({v.x,v.z}).get(), Angle({v.y,v.z}).get()); },
				[](auto& v){ return Vec2(Angle({-v.x,-v.z}).get(), Angle({v.y,-v.z}).get()); }
			};
			std::pair<rs::CubeFace, Vec2> GetCubeUV(const Vec3& dir) {
				auto id = GetCubeFaceId(dir);
				return std::make_pair(id,
							Anglefunc[static_cast<int>(id)](dir) / (spn::RadF::OneRotationAng/4));
			}
			Vec3 GetCubeDir(rs::CubeFace face, const Vec2& uv) {
				const auto tmpUv = Vec2(uv.x*2-1,
										uv.y*2-1);
				Vec3 ret;
				switch(face) {
					case rs::CubeFace::NegativeX:
						ret = Vec3(-1, tmpUv.y, tmpUv.x);
						break;
					case rs::CubeFace::PositiveX:
						ret = Vec3(1, tmpUv.y, 1-tmpUv.x);
						break;
					case rs::CubeFace::NegativeY:
						ret = Vec3(tmpUv.x, -1, tmpUv.y);
						break;
					case rs::CubeFace::PositiveY:
						ret = Vec3(tmpUv.x, 1, 1-tmpUv.y);
						break;
					case rs::CubeFace::NegativeZ:
						ret = Vec3(1-tmpUv.x, tmpUv.y, -1);
						break;
					case rs::CubeFace::PositiveZ:
						ret = Vec3(tmpUv.x, tmpUv.y, 1);
						break;
					default:
						AssertF(Trap, "")
				}
				return ret.normalization();
			}

			/*!
				[  ][Y+][  ][  ]
				[X-][Z+][X+][Z-]
				[  ][Y-][  ][  ]
			*/
			Vec2 GetCubeUnwrapUV(rs::CubeFace face, const Vec2& uv) {
				const Vec2 offset[static_cast<int>(rs::CubeFace::Num)] = {
					{2,1}, {0,1},
					{1,0}, {1,2},
					{3,1}, {1,1}
				};
				const Vec2 unit(1.f/4,
								1.f/3);
				return (offset[static_cast<int>(face)] + uv) * unit;
			}
		}
		void Geometry::UVUnwrapCube(Vec2V& dstUv,
									const Pose3D& pose,
									const Vec3V& srcPos,
									const IndexV& srcIndex)
		{
			const int nV = srcPos.size();
			auto toLocal = pose.getToLocal();
			SumVecV<3> sv(nV);
			Vec3V pos(nV);
			for(int i=0 ; i<nV ; i++) {
				pos[i] = srcPos[i].asVec4(1) * toLocal;
				pos[i].normalize();
			}
			const int nI = srcIndex.size();
			for(int i=0 ; i<nI ; i+=3) {
				auto* idx = srcIndex.data()+i;
				auto c = (pos[idx[0]] + pos[idx[1]] + pos[idx[2]]) / 3;
				for(int j=0 ; j<3 ; j++)
					sv[idx[j]].add(c);
			}
			dstUv.resize(nV);
			for(int i=0 ; i<nV ; i++) {
				auto nml = sv[i].getNormal();
				auto& func = Anglefunc[static_cast<int>(GetCubeFaceId(nml))];
				dstUv[i] = func(pos[i]) * 0.5f + Vec2(0.5f);
			}
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

			SumVecV<3> vn(nV);
			for(int i=0 ; i<nI ; i+=3) {
				// 面法線を計算
				auto i0 = srcIndex[i],
					i1 = srcIndex[i+1],
					i2 = srcIndex[i+2];
				auto &p0 = srcPos[i0],
					&p1 = srcPos[i1],
					&p2 = srcPos[i2];
				auto p = Plane::FromPts(p0, p1, p2);
				auto nml = p.getNormal();
				// 角度によるウェイト
				vn[i0].add(nml * CalcAngle(p0, p1, p2).get());
				vn[i1].add(nml * CalcAngle(p1, p2, p0).get());
				vn[i2].add(nml * CalcAngle(p2, p0, p1).get());
			}
			// 頂点法線を計算
			for(int i=0 ; i<nV ; i++) {
				dstNormal[i] = vn[i].getNormal();
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
		void Geometry::CalcTangent(Vec4V& dstTan,
									Vec3V& srcPos,
									IndexV& srcIndex,
									Vec3V& srcNormal,
									Vec2V& srcUv)
		{
			int nI = srcIndex.size(),
				nV = srcPos.size();
			AssertT(Trap, nI%3==0, std::invalid_argument, "Invalid face count")
			auto fnCalc = [](auto& p, auto& q, auto& uvd0, auto& uvd1) -> Vec3x2_OP {
				const float s0 = uvd0.x,
							s1 = uvd1.x,
							t0 = uvd0.y,
							t1 = uvd1.y;
				// UV座標が隣の頂点とほぼ変わらない場合は接空間を計算しない
				if(std::abs(s0) + std::abs(t0) < 1e-7f)
					return spn::none;
				if(std::abs(s1) + std::abs(t1) < 1e-7f)
					return spn::none;
				const float c = 1.f/(s0*t1 - s1*t0);
				Mat22 m(t1, -t0, -s1, s0);
				m *= c;
				Mat23 m2;
				m2.setRow(0, p);
				m2.setRow(1, q);

				Mat23 m3;
				m3 = m * m2;
				auto vt = m3.getRow(0),
					vb = m3.getRow(1);
				return Vec3x2(std::make_pair(vt, vb));
			};
			struct Vec3x3 {
				Vec3 vec[3];
			};
			using Vec3x3V = std::vector<Vec3x3>;
			// PolyFace毎のTangentVector
			Vec3x3V tangent(nI/3);
			// 頂点毎の使用されているPolyFaceId
			using Used = std::vector<std::pair<int, float>>;
			std::vector<Used> usedBy(nV);
			for(int i=0 ; i<nI ; i+=3) {
				auto i0 = srcIndex[i],
					i1 = srcIndex[i+1],
					i2 = srcIndex[i+2];
				auto &p0 = srcPos[i0],
					&p1 = srcPos[i1],
					&p2 = srcPos[i2];
				const Vec3 p = p1 - p0,
							q = p2 - p0;
				const Vec2 uvd0 = srcUv[i1] - srcUv[i0],
							uvd1 = srcUv[i2] - srcUv[i0];
				// 三角形の面積がほぼゼロ場合は接空間を計算しない
				float area = boom::Area_x2(p, q);
				if(area < 1e-6f)
					continue;
				if(auto ret = fnCalc(p, q, uvd0, uvd1)) {
					// 面積に応じたウェイト
					auto vt = ret->first * area,
						 vb = ret->second * area;
					auto nml = spn::Plane::FromPts(p0, p1, p2).getNormal();
					tangent[i/3] = {{vt, vb, nml}};

					// 角度に応じたウェイト
					usedBy[i0].emplace_back(i/3, CalcAngle(p0,p1,p2).get());
					usedBy[i1].emplace_back(i/3, CalcAngle(p1,p2,p0).get());
					usedBy[i2].emplace_back(i/3, CalcAngle(p2,p0,p1).get());
				} else
					tangent[i/3] = {};
			}
			auto fnClone = [](auto& vec, int id) {
				auto tmp = vec[id];
				vec.push_back(std::move(tmp));
				return vec.size()-1;
			};
			auto fnCloneVertex = [&fnClone, &srcPos, &srcUv, &srcNormal](int id) -> int {
				fnClone(srcPos, id);
				fnClone(srcUv, id);
				return fnClone(srcNormal, id);
			};
			// 急激にTangentVectorが変わっているような所は頂点を分割する
			for(int i=0 ; i<nV ; i++) {
				auto& u = usedBy[i];
				const int nU = u.size();
				bool valid = true;
				for(int j=0 ; j<nU-1 ; j++) {
					auto& t0 = tangent[u[j].first];
					for(int k=j+1 ; k<nU ; k++) {
						auto& t1 = tangent[u[k].first];
						if(t0.vec[0].dot(t1.vec[0]) <= 0 ||
							t0.vec[1].dot(t1.vec[1]) <= 0)
						{
							valid = false;
							j=nU;
							break;
						}
					}
				}
				if(!valid) {
					for(int j=1 ; j<nU ; j++) {
						auto& u2 = usedBy[i][j];
						// 頂点を複製
						const int pidx = u2.first * 3;
						// インデックスの付け替え
						bool proc = false;
						for(int k=0 ; k<3 ; k++) {
							if(srcIndex[pidx+k] == i) {
								const int newVId = fnCloneVertex(i);
								srcIndex[pidx+k] = newVId;
								usedBy.resize(usedBy.size()+1);
								usedBy.back().push_back(u2);
								proc = true;
								break;
							}
						}
						Assert(Trap, proc)
					}
					usedBy[i].resize(1);
				}
			}
			auto nV2 = srcPos.size();
			Assert(Trap, usedBy.size()==nV2 &&
						srcUv.size()==nV2 &&
						srcNormal.size()==nV2)

			nV = srcPos.size();
			dstTan.resize(nV);
			for(int i=0 ; i<nV ; i++) {
				auto& u = usedBy[i];
				Vec3 x(0),
					 y(0);
				for(auto& u2 : u) {
					auto& t = tangent[u2.first];
					x += t.vec[0] * u2.second;
					y += t.vec[1] * u2.second;
				}
				const Vec3& z = srcNormal[i];
				x -= z*x.dot(z);
				y -= z*y.dot(z) + x*y.dot(x);
				x.normalize();
				// 掌性をw成分にセット
				auto c = ((z % x).dot(y) > 0.f) ? 1.f : -1.f;
				dstTan[i] = x.asVec4(c);
			}
		}
	}
	namespace geo2d {
		void Geometry::MakeRect(Vec2V& dstPos,
							IndexV& dstIndex)
		{
			dstPos = {Vec2(0,0),
						Vec2(0,1),
						Vec2(1,1),
						Vec2(1,0)};
			dstIndex = {0,1,2, 2,3,0};
		}
		void Geometry::MakeArc(Vec2V& dstPos,
							IndexV& dstIndex,
							const spn::RadF angle,
							const int div,
							const bool bCap)
		{
			AssertT(Trap, div>=2, std::invalid_argument, "Invalid division number")
			const int nV = 2+div;
			{
				dstPos.resize(nV);
				auto* pV = dstPos.data();
				*pV++ = Vec2(0,0);
				const float diff = angle.get()/div;
				float cur = 0;
				for(int i=0 ; i<div ; i++) {
					*pV++ = Vec2(std::sin(cur), std::cos(cur));
					cur += diff;
				}
				Assert(Trap, pV==dstPos.data()+nV)
			}
			{
				const int nI = (nV-2)*3 - ((bCap) ? 0 : 3);
				dstIndex.resize(nI);
				auto* pI = dstIndex.data();
				auto add3 = MakeAdd3(pI);
				for(int i=0 ; i<div ; i++)
					add3(0,i,i+1);
				if(bCap)
					add3(nV-2, nV-1, 0);
				Assert(Trap, pI==dstIndex.data()+nI)
			}
		}
		void Geometry::MakeCircle(Vec2V& dstPos,
								IndexV& dstIndex,
								const int div)
		{
			MakeArc(dstPos, dstIndex, spn::RadF(spn::PI*2/div * (div-1)), div-1, true);
		}
		void Geometry::MakeCapsule(Vec2V& dstPos,
								IndexV& dstIndex,
								const int div)
		{
			// 右半分
			MakeArc(dstPos, dstIndex, spn::RadF(spn::PI), div, false);
			const int nI = dstIndex.size();
			dstIndex.resize(nI*2 + 6);

			// 左半分 = 右半分のX軸反転
			const int nV = dstPos.size();
			dstPos.resize(nV*2);
			for(int i=0 ; i<nV ; i++)
				dstPos[i+nV] = dstPos[i] * Vec2(-1,1);
			FlipFace(dstIndex.begin(), dstIndex.begin()+nI, dstIndex.begin()+nI, nV);

			// 中間部分
			const int baseR[] = {1, 1+div};
			const int baseL[] = {baseR[1]+2, baseR[1]+2+div};
			auto* pI = dstIndex.data() + (nI*2);
			auto add4 = MakeAdd4(pI);
			add4(baseL[0], baseR[0], baseR[1], baseL[1]);
		}
	}
}
