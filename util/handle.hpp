//! リソースハンドルの前方宣言
#pragma once
#include "../handle.hpp"
#include "idmgr.hpp"
#include <utility>

namespace rs {
	namespace util {
		struct GeomId;
		using GeomIdMgr = IdMgr<GeomId>;
		struct GeomP {
			HLVb first;
			HLIb second;
		};
		class SharedGeomM;
		DEF_NHANDLE_PROP(SharedGeomM, Geom, GeomP, GeomP, std::allocator, typename GeomIdMgr::Id)
	}
}
