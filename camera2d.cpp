#include "camera2d.hpp"

namespace rs {
	Camera2D::Camera2D() {
		setAccum(1);
		setAspectRatio(1.f);
	}
	spn::RFlagRet Camera2D::_refresh(uint32_t& acc, Accum*) const {
		// キャッシュ更新
		getPose();
		getAspectRatio();
		// カウンタをインクリメント
		++acc;
		return {};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, View*) const {
		auto& ps = getPose();
		m = ps.getToLocal().convert33();
		return {};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewInv*) const {
		getView().inversion(m);
		return {};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, Proj*) const {
		m = spn::AMat33::Scaling(1.f / getAspectRatio(), 1, 1);
		return {};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewProj*) const {
		m = getView() * getProj();
		return {};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewProjInv*) const {
		getViewProj().inversion(m);
		return {};
	}

	spn::Vec2 Camera2D::vp2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewProjInv()).asVec2();
	}
	spn::Vec2 Camera2D::v2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewInv()).asVec2();
	}
}

