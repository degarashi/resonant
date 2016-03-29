#include "displacement.hpp"
#include "../glresource.hpp"
#include "test.hpp"

namespace {
	const rs::IdValue T_Tile = rs::IEffect::GlxId::GenTechId("Tile", "Default");
	const rs::IdValue U_Repeat = rs::IEffect::GlxId::GenUnifId("u_repeat"),
					U_Scale = rs::IEffect::GlxId::GenUnifId("u_scale"),
					U_Offset = rs::IEffect::GlxId::GenUnifId("u_tileoffset");
}
// -------------------- TileField --------------------
TileField::TileField(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
					const float scale, const float height, const float height_att, const float th, const float mv)
{
	auto h = Displacement::MakeDisplacement(rd, n, height_att);
	Displacement::Smooth(h, n, th, mv);
	const int tw = n / vn;
	_tile.resize(tw*tw);
	for(int i=0 ; i<tw ; i++) {
		for(int j=0 ; j<tw ; j++) {
			_tile[i*tw+j] = spn::construct(h, j*vn, i*vn, vn, n+1);
		}
	}
	_index = Displacement::MakeBuffer(Displacement::MakeIndex(vn));
	setStateNew<St_Default>();

	_center = spn::Vec3(0);
	_width = scale / tw;
	_tileWidth = tw;
	_nLevel = _index.size();
	_scale = {scale, height, scale};
	setViewDistanceCoeff(0.1f, 1.f);
	setTextureRepeat(1.f);
}
void TileField::setViewPos(const spn::Vec3& p) {
	_center = p;
}
void TileField::setViewDistanceCoeff(float dMin, float dMax) {
	_dMin = dMin * _width * _tileWidth;
	_dMax = dMax * _width * _tileWidth;
}
void TileField::setTexture(rs::HTex hTex) {
	_hlTex = hTex;
	_hlTex->get()->setUVWrap(rs::WrapState::MirroredRepeat, rs::WrapState::MirroredRepeat);
}
void TileField::setTextureRepeat(float r) {
	_repeat = r;
}
#include "engine.hpp"
#include "../sys_uniform_value.hpp"
struct TileField::St_Default : StateT<St_Default> {
	void onDraw(const TileField& self, rs::IEffect& e) const override {
		e.setTechPassId(T_Tile);
		e.setVDecl(rs::DrawDecl<vdecl::tile>::GetVDecl());
		auto& engine = static_cast<Engine&>(e);
		engine.ref3D().setWorld(spn::Mat44(spn::Mat44::TagIdentity));
		if(self._hlTex)
			e.setUniform(rs::unif::texture::Diffuse, self._hlTex);
		e.setUniform(U_Repeat, self._repeat);
		const auto fnLevel = [
			dMin=self._dMin,
			dMax=self._dMax,
			pos=self._center,
			nLevel=self._nLevel
			] (float x, float y) -> int{
				float d = pos.distance(spn::Vec3(x,0,y));
				if(d <= dMin)
					return 0;
				if(nLevel<=2 || d>=dMax)
					return nLevel-1;
				d = (d-dMin) / (dMax-dMin);
				return (d * (nLevel-2))+1;
			};
		const float tw = self._width;
		const int s = self._tileWidth;
		const int s2 = s+2;
		std::vector<int> levels(s2*s2);
		for(auto& l : levels)
			l = 0;
		for(int i=0 ; i<s ; i++) {
			for(int j=0 ; j<s ; j++) {
				levels[(i+1)*s+(j+1)] = fnLevel(tw*j+tw/2, -tw*i-tw/2);
			}
		}
		e.setUniform(U_Scale, self._scale);
		for(int i=0 ; i<s ; i++) {
			for(int j=0 ; j<s ; j++) {
				e.setUniform(U_Offset, spn::Vec3(float(j)/s, 0, -float(i)/s));
				self._tile[i*s+j]->draw(e, self._index,
										levels[(i+1)*s+(j+1)],
										levels[(i+1)*s+j],
										levels[i*s+(j+1)],
										levels[(i+1)*s+(j+2)],
										levels[(i+2)*s+(j+1)]
				);
			}
		}
	}
};
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TileField, TileField, "DrawableObj", NOTHING,
		(setTexture)
		(setTextureRepeat)
		(setViewPos)
		(setViewDistanceCoeff),
		(spn::MTRandom&)(int)(int)(float)(float)(float)(float)(float))

// ---------------------- Tile頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::tile>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::POSITION},
		{0,8, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},
		{0,16, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL},

		{1,0, GL_FLOAT, GL_FALSE, 1, (GLuint)rs::VSem::TEXCOORD1}
	});
	return vd;
}
