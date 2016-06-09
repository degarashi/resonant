#include "textdraw.hpp"
#include "../sys_uniform.hpp"
#include "../camera.hpp"
#include "../glx.hpp"

namespace rs {
	namespace util {
		// ---------------------- Text3D ----------------------
		Text3D::Text3D(float lh, bool bBillboard):
			_lineHeight(lh),
			_bBillboard(bBillboard)
		{}
		void Text3D::setLineHeight(float lh) {
			_lineHeight = lh;
		}
		void Text3D::setBillboard(bool b) {
			_bBillboard = b;
		}
		int Text3D::draw(IEffect& e, bool bRefresh) const {
			auto& su3d = e.ref3D();
			auto cid = getCCoreId();
			float s = float(_lineHeight) / cid.at<CCoreID::Height>();
			auto mScale = spn::AMat44::Scaling(s, s, s, 1);
			mScale *= getToWorld().convertA44();
			return Text::draw(e, [&,bRefresh,s](auto&){
				spn::AMat44 m;
				if(_bBillboard) {
					// Poseの位置とスケーリングだけ取って
					// 向きはカメラに正対するように補正
					// Y軸は上
					auto& pose = su3d.getCamera()->getPose();
					auto& sc = getScale();
					auto m0 = spn::Mat44::Scaling(sc.x, sc.y, 1, 1);
					auto m1 = spn::Mat44::LookDirLH(spn::Vec3(0), pose.getDir(), pose.getUp());
					auto m2 = m1.transposition();
					m = mScale * m0 * m2;
				} else
					m = mScale * getToWorld().convertA44();
				su3d.setWorld(m);
				if(bRefresh)
					su3d.outputUniforms(e);
			});
		}
	}
}
