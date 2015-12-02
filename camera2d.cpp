#include "camera2d.hpp"

namespace rs {
	Camera2D::Camera2D() {
		setAccum(1);
		setAspectRatio(1.f);
	}
	spn::RFlagRet Camera2D::_refresh(typename Accum::value_type& acc, Accum*) const {
		auto ret = _rflag.getWithCheck(this, acc);
		auto& ps = *std::get<0>(ret.first);
		bool b = ret.second;
		if(b) {
			// カウンタをインクリメント
			++acc;
		}
		return {b, 0};
	}
	spn::RFlagRet Camera2D::_refresh(typename View::value_type& m, View*) const {
		auto ret = _rflag.getWithCheck(this, m);
		auto& ps = *std::get<0>(ret.first);
		bool b = ret.second;
		if(b)
			m = ps.getToLocal().convert33();
		return {b, 0};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewInv*) const {
		getView().inversion(m);
		return {true, 0};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, Proj*) const {
		m = spn::AMat33::Scaling(1.f / getAspectRatio(), 1, 1);
		return {true, 0};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewProj*) const {
		m = getView() * getProj();
		return {true, 0};
	}
	spn::RFlagRet Camera2D::_refresh(spn::Mat33& m, ViewProjInv*) const {
		getViewProj().inversion(m);
		return {true, 0};
	}

	spn::Vec2 Camera2D::vp2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewProjInv()).asVec2();
	}
	spn::Vec2 Camera2D::v2w(const spn::Vec2& pos) const {
		return (pos.asVec3(1) * getViewInv()).asVec2();
	}
	const std::string& Camera2DMgr::getResourceName(spn::SHandle /*sh*/) const {
		static std::string name("Camera2D");
		return name;
	}
}

