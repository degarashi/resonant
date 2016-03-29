#include "displacement.hpp"
#include "test.hpp"

// -------------------- Tile --------------------
template <class T>
T LoopValue(const T& t, const T& range) {
	if(t > range)
		return t-range;
	if(t < 0)
		return t+range;
	return t;
}
Tile::Tile(const Displacement::HeightL& h, const int ox, const int oy, const spn::PowInt size, const int stride) {
	using Vec2 = spn::Vec2;
	std::vector<vertex::tile_0> vtx((size+1)*(size+1));
	std::vector<vertex::tile_1> vtx2(vtx.size());
	auto fnAt = [&h, stride](int x, int y){
		return h[y*stride + x];
	};
	auto fnAtL = [&h, stride](int x, int y){
		return h[LoopValue<int>(y, stride)*stride + LoopValue<int>(x, stride)];
	};
	for(int i=0 ; i<=int(size) ; i++) {
		for(int j=0 ; j<=int(size) ; j++) {
			auto& v = vtx[i*(size+1)+j];
			auto& v2 = vtx2[i*(size+1)+j];
			v.pos.x = float(j)/(stride-1);
			v.pos.y = -float(i)/(stride-1);
			v2.height = fnAt(j+ox, i+oy);
			v.tex = Vec2(float(j+ox) / stride,
								float(i+oy) / stride);
			auto vL = fnAtL(ox+j-1,oy+i),
				 vR = fnAtL(ox+j+1,oy+i),
				 vT = fnAtL(ox+j,oy+i-1),
				 vB = fnAtL(ox+j,oy+i+1);
			spn::Vec3 xa(float(0.2f)/size, vR-vL, 0),
					za(0, vB-vT, float(0.2f)/size);
			v.normal = -xa.cross(za).normalization();
		}
	}
	_vertex[0] = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_vertex[0].ref()->initData(std::move(vtx));
	_vertex[1] = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_vertex[1].ref()->initData(std::move(vtx2));
}
namespace {
	template <class T>
		bool Equal(const T& t0, const T& t1) {
			return t0 == t1;
		}
	template <class T, class... Args>
		bool Equal(const T& t0, const T& t1, const Args&... args) {
			if(t0 != t1)
				return false;
			return Equal(t1, args...);
		}
}
void Tile::draw(rs::IEffect& e, const Displacement::IndexBuffV& idxv, const int center, const int left, const int top, const int right, const int bottom) const {
	e.setVStream(_vertex[0], 0);
	e.setVStream(_vertex[1], 1);
	auto fnDraw = [&e](rs::HIb ib, int offset, int offset_to){
		if(offset == offset_to)
			return;
		e.setIStream(ib);
		e.drawIndexed(GL_TRIANGLES, offset_to-offset, offset);
	};
	auto& idx = idxv[center];
	if(center < left ||
		center < right ||
		center < top ||
		center < bottom)
	{
		const int nmax = idx.dip.size()-1;
		using Dip = Displacement::Dip;
		auto fnDip = [&fnDraw, &idx, nmax, center](int level, Dip flag){
			level = std::min(nmax, level);
			level -= center;
			level = std::max(0, level);
			auto& dip = idx.dip[level];
			fnDraw(dip.ib, dip.offset[flag], dip.offset[flag+1]);
		};
		fnDip(left, Dip::Left);
		fnDip(top, Dip::Top);
		fnDip(right, Dip::Right);
		fnDip(bottom, Dip::Bottom);
		fnDip(center, Dip::Center);
	} else {
		// Full描画
		fnDraw(idx.full, 0, idx.full->get()->getNElem());
	}
}

