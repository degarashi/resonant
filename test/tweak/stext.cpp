#include "stext.hpp"

namespace tweak {
	namespace {
		const std::string cs_typename[] = {
			"Type",
			"Step",
			"Initial",
			"Current"
		};
	}
	STextPack::STextPack(const rs::CCoreID cid) {
		static_assert(countof(cs_typename) == Type::_Num, "");
		for(int i=0 ; i<int(countof(cs_typename)) ; i++) {
			htStatic[i] = mgr_text.createText(cid, cs_typename[i]);
		}
	}
}
