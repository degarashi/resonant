#pragma once
#include "handle.hpp"
#include "font.hpp"

namespace tweak {
	struct STextPack {
		enum Type {
			Type,
			Step,
			Initial,
			Current,
			_Num
		};
		rs::HLText	htStatic[Type::_Num];

		STextPack(rs::CCoreID cid);
	};
}
