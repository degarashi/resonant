#pragma once
#include "sharedhandle.hpp"
#include "../handle.hpp"

namespace rs {
	namespace util {
		template <class T, class Key=spn::none_t>
		class SharedGeometry {
			private:
				struct Vb_t : SharedHandle<Vb_t, Key, HVb> {
					template <class... Ts>
					HLVb _makeHandle(Ts&&... ts) const {
						return T::MakeVertex(std::forward<Ts>(ts)...);
					}
				};
				struct Ib_t : SharedHandle<Ib_t, Key, HIb> {
					template <class... Ts>
					HLIb _makeHandle(Ts&&... ts) const {
						return T::MakeIndex(std::forward<Ts>(ts)...);
					}
				};
				mutable HLVb	_hlVb;
				mutable HLIb	_hlIb;
			public:
				HVb getVertex(const Key& key=Key()) const {
					if(!_hlVb)
						_hlVb = Vb_t::GetHandle(key);
					return _hlVb;
				}
				HIb getIndex(const Key& key=Key()) const {
					if(!_hlIb)
						_hlIb = Ib_t::GetHandle(key);
					return _hlIb;
				}
		};
	}
}
