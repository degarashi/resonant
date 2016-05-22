#pragma once
#include "spinner/vector.hpp"

namespace rs {
	namespace util {
		struct ColorA : spn::Vec4 {
			ColorA() = default;
			ColorA(const spn::Vec3& c, float a=1);
			explicit ColorA(float a);

			void setColor(const spn::Vec3& v);
			void setColor(const spn::Vec4& v);
			void setAlpha(float a);
		};
	}
}
