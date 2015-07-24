#pragma once
#include "sharedgeom.hpp"
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
		//! [-1, 1]の正方形ジオメトリ
		class Rect01 : public SharedGeometry<Rect01> {
			public:
				static HLVb MakeVertex();
				static HLIb MakeIndex();
		};
		//! 画面全体を覆う矩形ポリゴン (ポストエフェクト用)
		class ScreenRect {
			private:
				Rect01	_rect01;
			public:
				void exportDrawTag(DrawTag& tag) const;
				void draw(GLEffect& e) const;
		};
	}
}
