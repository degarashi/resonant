#pragma once
#include "sharedgeom.hpp"
#include "../handle.hpp"
#include "../vertex.hpp"

#include "../glx_id.hpp"
#include "spinner/pose.hpp"

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
	class SystemUniform2D;
	namespace util {
		//! [0, 1]の正方形ジオメトリ
		class Rect01 : public SharedGeometry<Rect01> {
			public:
				static HLVb MakeVertex();
				static HLIb MakeIndex();
		};
		//! [-1, 1]の正方形ジオメトリ
		class Rect11 : public SharedGeometry<Rect11> {
			public:
				static HLVb MakeVertex();
				static HLIb MakeIndex();
		};
		//! 任意サイズの四角ポリゴン (デバッグ用)
		/*! サイズはスクリーン座標系で指定 */
		class WindowRect : public spn::Pose2D {
			private:
				Rect01		_rect01;
				spn::Vec3	_color;
				float		_alpha,
							_depth;
			public:
				WindowRect();
				void setColor(const spn::Vec3& c);
				void setAlpha(float a);
				void setDepth(float d);
				void exportDrawTag(DrawTag& tag) const;
				void draw(IEffect& e, SystemUniform2D& s2) const;
				template <class T>
				void draw(T& t) const { draw(t, t); }
		};
		//! 画面全体を覆う矩形ポリゴン (ポストエフェクト用)
		class ScreenRect {
			private:
				Rect11	_rect11;
			public:
				void exportDrawTag(DrawTag& tag) const;
				void draw(IEffect& e) const;
		};
	}
}
