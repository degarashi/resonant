#pragma once
#include "../handle.hpp"

namespace rs {
	struct DrawTag;
}
class Engine;
class ScreenRect {
	private:
		static rs::WVb		s_wVb;
		static rs::WIb		s_wIb;
		rs::HLVb			_hlVb;
		rs::HLIb			_hlIb;
	public:
		ScreenRect();
		void exportDrawTag(rs::DrawTag& tag) const;
		void draw(rs::GLEffect& e) const;
};
