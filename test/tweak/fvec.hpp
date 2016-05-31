#pragma once
#include "../luaw.hpp"

namespace tweak {
	struct FVec4 {
		int			nValue;
		spn::Vec4	value;

		FVec4() = default;
		FVec4(int nv, float iv);

		std::ostream& write(std::ostream& s) const;
		static FVec4 LoadFromLua(const rs::LValueS& v);
		rs::LCValue	toLCValue() const;
	};
}
