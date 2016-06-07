#pragma once
#include "spinner/singleton.hpp"

namespace rs {
	namespace util {
		template <class T>
		class IdMgr {
			public:
				using Id = uint_fast32_t;
			private:
				static Id s_id;
			public:
				static Id GenId() {
					Id ret = s_id;
					++s_id;
					return ret;
				}
				static void Initialize() { s_id = 0; }
				static void Terminate() {}
		};
		template <class T>
		typename IdMgr<T>::Id IdMgr<T>::s_id;
	}
}
