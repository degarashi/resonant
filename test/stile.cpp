#include "stile.hpp"
#include "test.hpp"

STile::STile(const Displacement::HeightL& h, int ox, int oy, spn::PowInt size, int stride) {
	_vertex[0] = Displacement::MakeTileVertex0(size);
	auto nml = Displacement::MakeTileVertexNormal(h, ox, oy, size, stride);
	// 四隅 = 0
	// そこから2等分 = 1
	auto calcLevelDetail = [size](int x){
		int level = 0,
			cur = size;
		for(;;) {
			if(x % cur == 0)
				return level;
			++level;
			cur >>= 1;
		}
	};
	auto fnAt = [&h, stride](int x, int y) {
		return h[y*stride+x];
	};
	enum Type {
		Horizontal,
		Vertical,
		Slanting
	};
	auto calcLevel = [&calcLevelDetail](int x, int y) -> std::pair<Type,int> {
		const int lx = calcLevelDetail(x),
					ly = calcLevelDetail(y);
		if(lx == ly) {
			// タイルの中央
			return {Slanting, lx};
		} else if(lx > ly) {
			return {Horizontal, lx};
		} else
			return {Vertical, ly};
	};
	const int s = size+1;
	auto fnAtNml = [&nml, s](int x, int y) {
		x = spn::Saturate(x, 0, s-1);
		y = spn::Saturate(y, 0, s-1);
		return nml[y*s+x];
	};
	std::vector<vertex::stile_1> vtx2(s*s);
	for(int i=0 ; i<s ; i++) {
		for(int j=0 ; j<s ; j++) {
			auto& v = vtx2[i*s+j];
			v.normalY = nml[i*s+j];
			v.height.y = fnAt(j+ox,i+oy);

			const auto nLevel = static_cast<float>(spn::Bit::MSB_N(size)+1);
			const auto lv = calcLevel(j,i);
			const int w = size >> lv.second;
			if(lv.second > 0) {
				switch(lv.first) {
					case Horizontal:
						v.height.x = (fnAt(j+ox+w, i+oy) + fnAt(j+ox-w, i+oy))/2;
						v.normalX = (fnAtNml(j+w, i) + fnAtNml(j-w, i)).normalization();
						break;
					case Vertical:
						v.height.x = (fnAt(j+ox,i+oy+w) + fnAt(j+ox,i+oy-w))/2;
						v.normalX = (fnAtNml(j,i+w) + fnAtNml(j,i-w)).normalization();
						break;
					case Slanting:
						v.height.x = (fnAt(j+ox+w,i+oy+w) + fnAt(j+ox-w,i+oy-w))/2;
						v.normalX = (fnAtNml(j+w,i+w) + fnAtNml(j-w,i-w)).normalization();
						break;
				}
				if(v.normalX.isOutstanding())
					v.normalX = spn::Vec3(0,1,0);
			} else {
				v.height.x = v.height.y;
				v.normalX = v.normalY;
			}
				Assert(Trap, !v.normalX.isOutstanding())
			v.height.z = nLevel - lv.second - 1;
		}
	}
	_vertex[1] = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_vertex[1].ref()->initData(std::move(vtx2));
}
#include "engine.hpp"
namespace {
	const rs::IdValue U_DistRange = rs::IEffect::GlxId::GenUnifId("u_distRange");
}
void STile::draw(rs::IEffect& e, const rs::HIb idx, const spn::Vec2& distRange) const {
	e.setVStream(_vertex[0], 0);
	e.setVStream(_vertex[1], 1);
	e.setIStream(idx);
	e.setUniform(U_DistRange, distRange);
	e.drawIndexed(GL_TRIANGLES, idx->get()->getNElem(), 0);
}

// ---------------------- STile頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::stile>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::POSITION},
		{0,8, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},

		{1,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::TEXCOORD1},
		{1,12, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::TEXCOORD2},
		{1,24, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::TEXCOORD3},
	});
	return vd;
}
const rs::SPVDecl& rs::DrawDecl<vdecl::collision>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::COLOR}
	});
	return vd;
}
