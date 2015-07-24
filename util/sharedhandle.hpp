#pragma once
#include "spinner/resmgr.hpp"

namespace rs {
	namespace util {
		template <class T, class Key, class Handle>
		class SharedHandle {
			private:
				using LHdl = spn::HdlLock<Handle, true>;
				using WHdl = typename Handle::WHdl;
				using Map = std::unordered_map<Key, WHdl>;
				static Map	s_map;
			public:
				static LHdl GetHandle(const Key& key) {
					WHdl& wh = s_map[key];
					LHdl lh = wh.lock();
					if(!lh) {
						lh = T()._makeHandle(key);
						wh = lh.weak();
					}
					return lh;
				}
		};
		template <class T, class Key, class Handle>
		typename SharedHandle<T,Key,Handle>::Map SharedHandle<T,Key,Handle>::s_map;

		template <class T, class Handle>
		class SharedHandle<T, spn::none_t, Handle> {
			private:
				using LHdl = spn::HdlLock<Handle, true>;
				using WHdl = typename Handle::WHdl;
				static WHdl s_wHdl;
			public:
				static LHdl GetHandle(...) {
					LHdl lh = s_wHdl.lock();
					if(!lh) {
						lh = T()._makeHandle();
						s_wHdl = lh.weak();
					}
					return lh;
				}
		};
		template <class T, class Handle>
		typename SharedHandle<T, spn::none_t, Handle>::WHdl SharedHandle<T,spn::none_t,Handle>::s_wHdl;
	}
}
