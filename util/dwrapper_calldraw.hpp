#pragma once
#include <functional>

namespace rs {
	class GLEffect;
	namespace util {
		template <class T>
		struct CallDraw {
			using Func = std::function<T& (GLEffect&)>;
			const Func	func;
			CallDraw(const Func& f): func(f) {}

			template <class T2>
			void operator()(const T2& t, GLEffect& e) const {
				t.draw(func(e));
			}
		};
	}
}
