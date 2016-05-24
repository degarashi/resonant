#include "clipmap.hpp"
#include "../glresource.hpp"
#include "../camera.hpp"

// --------------------- ClipmapObj ---------------------
struct ClipmapObj::St_Def : StateT<St_Def> {
	void onDraw(const ClipmapObj& self, rs::IEffect& e) const override {
		self.draw(e);
	}
};
ClipmapObj::ClipmapObj(spn::PowInt n, const int l, const int upsamp_n):
	Clipmap(n,l,upsamp_n)
{
	setStateNew<St_Def>();
}
const std::string& ClipmapObj::getName() const {
	static const std::string name("ClipmapObj");
	return name;
}
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Clipmap::DrawCount, DrawCount,
		(draw)(not_draw),
		NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ClipmapObj, ClipmapObj, "DrawableObj", NOTHING,
		(setRayleighCoeff)(setMieCoeff)(setLightDir)(setLightColor)(setLightPower)(setDivide)
		(setGridSize)(setCamera)(getCache)(getNormalCache)(getDrawCount)(setGridSize)(setDiffuseSize)
		(setPNElevation)(setWaveElevation),
		(int)(int)(int)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(IClipSource_SP, IClipSource_SP, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(HashVec_SP, HashVec_SP, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(Hash_SP, Hash_SP, NOTHING, NOTHING)
