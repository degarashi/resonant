#include "tile.hpp"
#include "../glresource.hpp"
#include "test.hpp"

namespace {
	const rs::IdValue T_Tile = rs::IEffect::GlxId::GenTechId("Tile", "Default");
}
// -------------------- TileField --------------------
TileField::TileField(spn::MTRandom& rd, const spn::PowInt n, const spn::PowInt vn,
					const float scale, const float height, const float height_att, const float th, const float mv): base_t(rd, n, vn, scale, height, height_att, th, mv)
{
	const int tw = _tileWidth;
	_tile.resize(tw*tw);
	for(int i=0 ; i<tw ; i++) {
		for(int j=0 ; j<tw ; j++) {
			_tile[i*tw+j] = spn::construct(_heightL, j*vn, i*vn, vn, n+1);
		}
	}
	setStateNew<St_Default>();
}
#include "engine.hpp"
#include "../sys_uniform_value.hpp"
struct TileField::St_Default : StateT<St_Default> {
	void onDraw(const TileField& self, rs::IEffect& e) const override {
		e.setTechPassId(T_Tile);
		e.setVDecl(rs::DrawDecl<vdecl::tile>::GetVDecl());
		self._prepareValues(e);

		const int s = self._tileWidth;
		const int s2 = s+2;
		std::vector<int> levels(s2*s2);
		for(auto& l : levels)
			l = 0;
		for(int i=0 ; i<s ; i++) {
			for(int j=0 ; j<s ; j++) {
				levels[(i+1)*s+(j+1)] = self._calcLevel(j+0.5f, i+0.5f);
			}
		}
		for(int i=0 ; i<s ; i++) {
			for(int j=0 ; j<s ; j++) {
				e.setUniform(U_Offset, spn::Vec3(j, 0, -i));
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
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TileField, TileField, "TileFieldBase", NOTHING,
		NOTHING,
		(spn::MTRandom&)(int)(int)(float)(float)(float)(float)(float))

// ---------------------- Tile頂点宣言 ----------------------
const rs::SPVDecl& rs::DrawDecl<vdecl::tile>::GetVDecl() {
	static rs::SPVDecl vd(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::POSITION},
		{0,8, GL_FLOAT, GL_FALSE, 2, (GLuint)rs::VSem::TEXCOORD0},

		{1,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::NORMAL},
		{1,12, GL_FLOAT, GL_FALSE, 1, (GLuint)rs::VSem::TEXCOORD1}
	});
	return vd;
}
