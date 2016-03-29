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
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_PTR_NOCTOR(Clipmap::DrawCount, DrawCount,
		(draw)(not_draw),
		NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ClipmapObj, ClipmapObj, "DrawableObj", NOTHING,
		(setGridSize) (setCamera)(getCache)(getNormalCache)(getDrawCount),
		(int)(int)(int)
)
