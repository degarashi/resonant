#include "camera2d.hpp"
#include "glx.hpp"

namespace rs {
	using GlxId = rs::GLEffect::GlxId;

	Camera2D::Camera2D() {
		setAccum(1);
		setAspectRatio(1.f);
	}
	uint32_t Camera2D::_refresh(uint32_t& acc, Accum*) const {
		// キャッシュ更新
		getPose();
		getAspectRatio();
		// カウンタをインクリメント
		++acc;
		return 0;
	}
	uint32_t Camera2D::_refresh(spn::Mat33& m, View*) const {
		auto& ps = getPose();
		m = ps.getToLocal().convert33();
		return 0;
	}
	uint32_t Camera2D::_refresh(spn::Mat33& m, ViewInv*) const {
		getView().inversion(m);
		return 0;
	}
	uint32_t Camera2D::_refresh(spn::Mat33& m, Proj*) const {
		m = spn::AMat33::Scaling(1.f * getAspectRatio(), 1, 1);
		return 0;
	}
	uint32_t Camera2D::_refresh(spn::Mat33& m, ViewProj*) const {
		m = getView() * getProj();
		return 0;
	}
	uint32_t Camera2D::_refresh(spn::Mat33& m, ViewProjInv*) const {
		getViewProj().inversion(m);
		return 0;
	}

	spn::Vec2 Camera2D::vp2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewProjInv()).asVec2();
	}
	spn::Vec2 Camera2D::v2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewInv()).asVec2();
	}
}

