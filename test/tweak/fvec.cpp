#include "test/tweak/fvec.hpp"

namespace tweak {
	// ---------------- FVec4 ----------------
	FVec4::FVec4(const int nv, const float iv):
		nValue(nv),
		value(iv)
	{}
	FVec4 FVec4::LoadFromLua(const rs::LValueS& v) {
		FVec4 ret;
		auto& value = ret.value;
		int cur = 0;
		auto fn = [&v, &cur](auto& dst, const auto& key){
			const rs::LValueS val = v[key];
			if(val.type() == rs::LuaType::Number) {
				++cur;
				dst = val.template toValue<float>();
				return true;
			}
			return false;
		};
		if(v.type() == rs::LuaType::Number) {
			value.x = v.toValue<float>();
			cur = 1;
		} else if(fn(value.m[0], rs::LCValue(1))) {
			if(fn(value.m[1], 2))
				if(fn(value.m[2], 3))
					fn(value.m[3], 4);
		} else {
			fn(value.x, "x");
			if(fn(value.y, "y"))
				if(fn(value.z, "z"))
					fn(value.w, "w");
		}
		ret.nValue = cur;
		return ret;
	}
	rs::LCValue	FVec4::toLCValue() const {
		switch(nValue) {
			case 2: return spn::Vec2(value.x, value.y);
			case 3: return spn::Vec3(value.x, value.y, value.z);
			case 4: return value;
		}
		return value.x;
	}
	std::ostream& FVec4::write(std::ostream& s) const {
		bool bF = true;
		const int nv = nValue;
		if(nv == 1)
			return s << value.x;

		s << '{';
		for(int i=0 ; i<nv ; i++) {
			if(!bF)
				s << ',';
			bF = false;
			s << value.m[i];
		}
		return s << '}';
	}
}
