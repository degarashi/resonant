#include "color.hpp"

namespace rs {
	namespace util {
		ColorA::ColorA(const float a):
			spn::Vec4(1,1,1,a)
		{}
		ColorA::ColorA(const spn::Vec3& c, const float a):
			spn::Vec4(c.x, c.y, c.z, a)
		{}
		void ColorA::setColor(const spn::Vec3& v) {
			reinterpret_cast<spn::Vec3&>(*this) = v;
		}
		void ColorA::setColor(const spn::Vec4& v) {
			static_cast<spn::Vec4&>(*this) = v;
		}
		void ColorA::setAlpha(const float a) {
			w = a;
		}
	}
}
