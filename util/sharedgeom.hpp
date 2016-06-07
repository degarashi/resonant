#pragma once
#include "util/handle.hpp"

namespace rs {
	namespace util {
		class SharedGeomM : public spn::ResMgrN<GeomP, SharedGeomM, std::allocator, typename GeomIdMgr::Id> {
			public:
				using Id = GeomIdMgr::Id;
		};
		#define mgr_geom (::rs::util::SharedGeomM::_ref())

		template <class T>
		class SharedGeom {
			private:
				HLGeom _hl;
				const static GeomIdMgr::Id cs_gid;
			public:
				SharedGeom():
					_hl(mgr_geom.acquire(cs_gid, &T::MakeGeom).first)
				{}
				const GeomP& getGeom() const {
					return _hl.cref();
				}
		};
		#define mgr_geomid ::rs::util::GeomIdMgr
		template <class T>
		const GeomIdMgr::Id SharedGeom<T>::cs_gid = mgr_geomid::GenId();
	}
}
