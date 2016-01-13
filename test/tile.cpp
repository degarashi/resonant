#include "tile.hpp"
#include "test.hpp"

// -------------------- Tile --------------------
Tile::Tile(const Displacement::HeightL& h, const int ox, const int oy, const spn::PowInt size, const int stride) {
	_vertex[0] = Displacement::MakeTileVertex0(size);

	const int s = size+1;
	auto nml = Displacement::MakeTileVertexNormal(h, ox, oy, size, stride);
	std::vector<vertex::tile_1> vtx2(s*s);
	auto fnAt = [&h, stride](int x, int y){
		return h[y*stride + x];
	};
	for(int i=0 ; i<s ; i++) {
		for(int j=0 ; j<s ; j++) {
			auto& v2 = vtx2[i*s+j];
			v2.height = fnAt(j+ox, i+oy);
			v2.normal = nml[i*s+j];
		}
	}
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

