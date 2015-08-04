#pragma once
#include <functional>

namespace rs {
	struct IEffect;
	namespace util {
		template <class T>
		struct CallDraw {
			using Func = std::function<T& (IEffect&)>;
			const Func	func;
			CallDraw(const Func& f): func(f) {}

			template <class T2>
			void operator()(const T2& t, IEffect& e) const {
				t.draw(func(e));
			}
		};
	}
}
