#include "textdraw.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform.hpp"
#include "../sys_uniform_value.hpp"

namespace rs {
	namespace util {
		// ---------------------- Text2D ----------------------
		Text2D::Text2D(float lh):
			_lineHeight(lh),
			_depth(1)
		{}
		void Text2D::setLineHeight(float lh) {
			_lineHeight = lh;
		}
		void Text2D::setDepth(float d) {
			_depth = d;
		}
		int Text2D::draw(IEffect& e, bool bRefresh) const {
			auto& su2d = e.ref2D();
			auto cid = getCCoreId();
			// Zが0.0未満や1.0以上だと描画されないので、それより少し狭い範囲でクリップする
			float d = spn::Saturate(_depth, 0.f, 1.f-1e-4f);
			float s = float(_lineHeight) / cid.at<CCoreID::Height>();
			auto m = spn::AMat33::Scaling(s, s, 1);
			m *= getToWorld().convertA33();
			return Text::draw(e, [d, &e, &su2d, &m, bR=bRefresh](auto&){
				e.setUniform(unif2d::Depth, d);
				su2d.setWorld(m);
				if(bR)
					su2d.outputUniforms(e);
			});
		}
	}
}
