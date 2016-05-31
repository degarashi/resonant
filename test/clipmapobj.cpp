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
