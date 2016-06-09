#include "test/tweak/common.hpp"
#include "../../luaw.hpp"
#include "../../util/color.hpp"
namespace tweak {
	// ---------------- Color ----------------
	namespace {
		const rs::util::ColorA cs_color[Color::_Num] = {
			{{1,1,1}},
			{{1,1,0.5f}},
			{{1,0.5f,1}},
			{{0.5f,1,1}},
			{{0.5f, 0.5f, 1}},
			{{1,1,1}}
		};
	}
	const rs::util::ColorA& Color::get() const {
		return cs_color[c];
	}
	// ---------------- IBase ----------------
	void IBase::set(const LValueS&, bool) {
		Assert(Warn, false, "invalid function call")
	}
	void IBase::setInitial(const LValueS&) {
		Assert(Warn, false, "invalid function call")
	}
	void IBase::increment(float,int) {
		Assert(Warn, false, "invalid function call")
	}
	int IBase::draw(const Vec2&,  const Vec2&, Drawer&) const {
		AssertF(Trap, "invalid function call")
	}
	rs::LCValue IBase::get() const {
		Assert(Warn, false, "invalid function call")
		return rs::LCValue();
	}
}
