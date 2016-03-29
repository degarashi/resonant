#include "displacement.hpp"
#include "spinner/random.hpp"

void Displacement::Smooth(HeightL& h, const spn::PowInt n, const float th, const float mv) {
	const int s = n+1;
	struct Avg {
		float	average,
				maxDiff,
				center;
	};
	auto fnAvg = [&h, s](int x, int y) -> Avg {
		auto vL = h[y*s + ((x-1<0) ? x+1 : x)];
		auto vR = h[y*s + ((x+1>=s) ? x-1 : x)];
		auto vT = h[((y-1<0) ? y+1 : y)*s + x];
		auto vB = h[((y+1>=s) ? y-1 : y)*s + x];
		auto vC = h[y*s + x];
		return {(vL+vR+vT+vB)/4,
				std::max(std::max(std::abs(vL-vC), std::abs(vR-vC)),
						std::max(std::abs(vT-vC), std::abs(vB-vC))),
				vC};
	};
	HeightL tmp(s*s);
	for(int i=0 ; i<s ; i++) {
		for(int j=0 ; j<s ; j++) {
			auto a = fnAvg(j,i);
			if(a.maxDiff >= th)
				tmp[i*s+j] = a.center + (a.average - a.center) * mv;
			else
				tmp[i*s+j] = a.center;
		}
	}
	std::swap(h, tmp);
}
Displacement::HeightL Displacement::MakeDisplacement(spn::MTRandom& rd,
		const spn::PowInt n, const float height_att)
{
	const int s = n+1;
	HeightL ret(s*s);
	auto fnAt = [&ret, s](const int x, const int y) -> auto& {
		return ret[y*s + x];
	};
	auto fnAtL = [&ret, s, &fnAt](const int x, const int y) -> auto& {
		auto fnLoop = [s](const int v){
			if(v < 0)
				return 0;//v + s - 1;
			if(v >= s)
				return s-1;//v - s + 1;
			return v;
		};
		return fnAt(fnLoop(x), fnLoop(y));
	};
	// 四隅を初期化
	float diff = 1.f;
	auto fRand = rd.getUniformF<float>({0.f, diff});
	ret[0] = fRand();
	ret[s-1] = fRand();
	ret[s*(s-1)] = fRand();
	ret[s*s-1] = fRand();
	diff *= height_att;
	auto fnSquare = [&fnAt, &rd](const int ox, const int oy, const int w, const float diff){
		const int wh = w/2;
		float r = fnAt(ox, oy) +
					fnAt(ox+w, oy) +
					fnAt(ox+w, oy+w) +
					fnAt(ox, oy+w);
		fnAt(ox+wh, oy+wh) = r/4 + rd.template getUniform<float>({-diff/2, diff/2});
	};
	auto fnDiamond2 = [&rd, &fnAtL, &fnAt](const int ox, const int oy, const int w, const float diff){
		float r = fnAtL(ox-w, oy) +
					fnAtL(ox+w, oy) +
					fnAtL(ox, oy-w) +
					fnAtL(ox, oy+w);
		fnAt(ox, oy) = r/4 + rd.template getUniform<float>({-diff/2, diff/2});
	};
	auto fnDiamond = [&fnDiamond2](const int ox, const int oy, const int w, const float diff){
		const int wh = w/2;
		fnDiamond2(ox+wh, oy, wh, diff);
		fnDiamond2(ox+wh, oy+w, wh, diff);
		fnDiamond2(ox, oy+wh, wh, diff);
		fnDiamond2(ox+w, oy+wh, wh, diff);
	};
	int w = n;
	do {
		// square phase
		for(int i=0 ; i<int(n) ; i+=w) {
			for(int j=0 ; j<int(n) ; j+=w) {
				fnSquare(j, i, w, diff);
			}
		}
		// diamond phase
		for(int i=0 ; i<int(n) ; i+=w) {
			for(int j=0 ; j<int(n) ; j+=w) {
				fnDiamond(j, i, w, diff);
			}
		}
		w /= 2;
		diff *= height_att;
	} while(w != 0);
	return ret;
}
namespace {
	const int c_index[2][6] = {
		{0,1,3, 3,2,0},
		{0,1,2, 1,3,2}
	};
	struct XY {
		int		oxI, oyI,
				oxO, oyO,
				dx, dy;
		bool	bFlip;
	};
	template <class Dst>
	void MakeIndexPlane(Dst& dst, const int w, const int s, const int shrink) {
		auto proc = [&dst, w,s](const int (&ofs)[6], const spn::RangeI& xr, const spn::RangeI& yr) {
			for(int i=yr.from ; i<yr.to ; i+=w) {
				const int base0 = i*s,
						base1 = base0 + w*s;
				for(int j=xr.from ; j<xr.to ; j+=w) {
					const int idx[4] = {base0+j, base0+j+w, base1+j, base1+j+w};
					for(auto o : ofs)
						dst.push_back(idx[o]);
				}
			}
		};
		if(w > s/2) {
			if(shrink == 0) {
				dst.push_back(0);
				dst.push_back(s-1);
				dst.push_back(s*s-1);
				dst.push_back(s*s-1);
				dst.push_back(s*(s-1));
				dst.push_back(0);
			} else {
				dst.push_back(0);
				dst.push_back(0);
				dst.push_back(0);
			}
			return;
		}
		const spn::RangeI range[2] = {{w*shrink, s/2}, {s/2, s-w*shrink-1}};
		// 左上
		proc(c_index[0], range[0], range[0]);
		// 右上
		proc(c_index[1], range[1], range[0]);
		// 左下
		proc(c_index[1], range[0], range[1]);
		// 右下
		proc(c_index[0], range[1], range[1]);
	}
	void MakeIndexDipDetail(Displacement::OIndex& dst, const XY& xy, const int s, const int wi, const int wo) {
		if(wi > s/2) {
			for(int i=0 ; i<3 ; i++)
				dst.push_back(0);
			dst.finishGroup();
			return;
		}
		int idxI = xy.oxI + xy.oyI*s,
			idxO = xy.oxO + xy.oyO*s;
		const int dI = xy.dx*wi + xy.dy*wi*s,
				dO = xy.dx*wo + xy.dy*wo*s;
		int curI = xy.oxI*xy.dx + xy.oyI*xy.dy,
			curO = xy.oxO*xy.dx + xy.oyO*xy.dy;
		auto fnAdd = [s, &dst, flip=xy.bFlip](const int i0, const int i1, const int i2){
			dst.push_back(i0);
			if(!flip) {
				dst.push_back(i1);
				dst.push_back(i2);
			} else {
				dst.push_back(i2);
				dst.push_back(i1);
			}
		};
		for(;;) {
			if(curI >= curO+wo/2 || curI == s-wi-1) {
				idxO += dO;
				curO += wo;
				if(curO >= s)
					break;

				fnAdd(idxO-dO, idxO, idxI);
			} else {
				idxI += dI;
				curI += wi;
				if(curI >= s-wi+1)
					break;

				fnAdd(idxI, idxI-dI, idxO);
			}
		}
		dst.finishGroup();
	}
	void MakeIndexDip(Displacement::OIndex& dst, const spn::PowInt w,
						const int resolution, const int target_resolution, const Displacement::Dip dip)
	{
		const int s = w+1;
		const int ri = 1<<resolution,
					ro = 1<<target_resolution;
		if(dip < Displacement::Dip::Center) {
			const XY c_xy[] = {
				{ri,ri, 0,0, 0,1, true},
				{ri,ri, 0,0, 1,0, false},
				{int(w)-ri,ri, int(w),0, 0,1, false},
				{ri,int(w)-ri, 0,int(w), 1,0, true}
			};
			MakeIndexDipDetail(dst, c_xy[dip], s, ri, ro);
		} else {
			MakeIndexPlane(dst, ri, s, 1);
			dst.finishGroup();
		}
	}
}
Displacement::TileV Displacement::MakeIndex(const spn::PowInt w) {
	Assert(Trap, w>=1)
	const int nLevel = spn::Bit::MSB_N(w)+1,
			nSubLevel = nLevel;
	TileV tile(nLevel);
	for(int i=0 ; i<nLevel ; i++) {
		auto& t = tile[i];
		MakeIndexPlane(t.full, 1<<i, w+1, 0);
		t.dip.resize(nSubLevel);
		for(int j=0 ; j<nSubLevel-i ; j++) {
			auto& d = t.dip[j];
			for(int k=0 ; k<Dip::Num ; k++) {
				MakeIndexDip(d, w, i, i+j, static_cast<Displacement::Dip>(k));
			}
		}
	}
	return tile;
}

#include "../glresource.hpp"
Displacement::IndexBuffV Displacement::MakeBuffer(const TileV& tile) {
	auto fnMake = [](auto& ia){
		rs::HLIb ib = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
		ib.ref()->initData(ia);
		return ib;
	};
	const int nT = tile.size();
	IndexBuffV ret(nT);
	for(int i=0 ; i<nT ; i++) {
		auto& src = tile[i];
		auto& dst = ret[i];
		dst.full = fnMake(src.full);

		const int nD = src.dip.size();
		dst.dip.resize(nD);
		for(int j=0 ; j<nD ; j++) {
			auto& sd = src.dip[j];
			auto& dd = dst.dip[j];
			dd.ib = fnMake(sd.getArray());

			auto& ofs = sd.getOffset();
			for(int k=0 ; k<int(ofs.size()) ; k++)
				dd.offset[k] = ofs[k];
			for(int k=int(ofs.size()) ; k<=Dip::End ; k++)
				dd.offset[k] = sd.getArray().size();
		}
	}
	return ret;
}
