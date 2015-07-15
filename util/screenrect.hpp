#pragma once
#include "../handle.hpp"
#include "../vertex.hpp"

namespace rs {
	namespace vertex {
		//! ScreenRect描画用頂点
		struct screen {
			spn::Vec2	pos;
			spn::Vec2	tex;
		};
	}
	namespace vdecl {
		struct screen {};
	}
}
DefineVDecl(::rs::vdecl::screen)

namespace rs {
	struct DrawTag;
	namespace util {
		//! 画面全体を覆う矩形ポリゴン (ポストエフェクト用)
		class ScreenRect {
			private:
				static WVb		s_wVb;
				static WIb		s_wIb;
				HLVb			_hlVb;
				HLIb			_hlIb;
			public:
				ScreenRect();
				void exportDrawTag(DrawTag& tag) const;
				void draw(GLEffect& e) const;
		};
	}
}
