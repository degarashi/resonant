#include "camera.hpp"

namespace rs {
	using spn::AMat43; using spn::AMat44;
	using spn::Vec2; using spn::Vec3;
	using spn::AVec4;
	using spn::Quat; using spn::Plane;
	using boom::geo3d::Frustum;
	spn::RFlagRet Camera3D::_refresh(typename Accum::value_type& acc, Accum*) const {
		auto ret = _rflag.getWithCheck(this, acc);
		bool b = ret.second;
		if(b) {
			// カウンタをインクリメント
			++acc;
		}
		return {b, 0};
	}
	spn::RFlagRet Camera3D::_refresh(typename View::value_type& m, View*) const {
		auto ret = _rflag.getWithCheck(this, m);
		auto& ps = *std::get<0>(ret.first);
		bool b = ret.second;
		if(b) {
			// 回転を一端quatに変換
			const Quat& q = ps.getRot();
			m.identity();
			m.getRow(3) = -ps.getOffset();

			Quat tq = q.inverse();
			m *= tq.asMat33();
		}
		return {b, 0};
	}
	spn::RFlagRet Camera3D::_refresh(AMat44& m, Proj*) const {
		m = AMat44::PerspectiveFovLH(getFov(), getAspect(), getNearZ(), getFarZ());
		return {true, 0};
	}
	spn::RFlagRet Camera3D::_refresh(AMat44& m, ViewProj*) const {
		m = getView().convertA44() * getProj();
		return {true, 0};
	}
	spn::RFlagRet Camera3D::_refresh(AMat44& m, ViewProjInv*) const {
		getViewProj().inversion(m);
		return {true, 0};
	}
	spn::RFlagRet Camera3D::_refresh(Frustum& vf, VFrustum*) const {
		// UP軸とZ軸を算出
		AMat44 mat;
		getView().convertA44().inversion(mat);
		mat.getRow(3) = AVec4(0,0,0,1);
		AVec4 zAxis(0,0,1,0),
			yAxis(0,1,0,0);
		zAxis *= mat;
		yAxis *= mat;

		auto& ps = getPose();
		vf = Frustum(ps.getOffset(), zAxis.asVec3(), yAxis.asVec3(), getFov(), getFarZ(), getAspect());
		return {true, 0};
	}

	Camera3D::Camera3D() {
		refPose().setAll(Vec3(0,0,-10), Quat(0,0,0,1), Vec3(1,1,1));
		setAccum(1);

		setNearZ(1.f);
		setFarZ(1e5f);
		setFov(spn::DegF(90.0f));
		setAspect(1.4f / 1.f);
	}
	void Camera3D::setZPlane(float n, float f) {
		setNearZ(n);
		setFarZ(f);
	}
	Vec3 Camera3D::unproject(const Vec3& vsPos) const {
		const AMat44& mI = getViewProjInv();
		return (vsPos.asVec4(1) * mI).asVec3Coord();
	}
	Camera3D::Vec3x2 Camera3D::unprojectVec(const Vec2& vsPos) const {
		const AMat44& mI = getViewProjInv();
		return Vec3x2(
			(AVec4(vsPos.x, vsPos.y, 0, 1) * mI).asVec3Coord(),
			(AVec4(vsPos.x, vsPos.y, 1, 1) * mI).asVec3Coord()
		);
	}
	Vec3 Camera3D::vp2wp(const Vec3& vp) const {
		return (vp.asVec4(1) * getViewProjInv()).asVec3Coord();
	}
	Plane Camera3D::getNearPlane() const {
		auto& ps = getPose();
		Vec3 dir = ps.getDir();
		return Plane::FromPtDir(ps.getOffset() + dir*getNearZ(), dir);
	}
	// カメラの変換手順としては offset -> rotation だがPose3Dの変換は rotation -> offsetなので注意！
	Frustum Camera3D::getNearFrustum() const {
		Frustum fr;
		float t = std::tan(getFov().get()/2);
		fr.setScale({t*getAspect(), t, getNearZ()*8});
		auto& ps = getPose();
		fr.setRot(ps.getRot());
		fr.setOffset(ps.getOffset());
		return fr;
	}
	const std::string& Camera3DMgr::getResourceName(spn::SHandle /*sh*/) const {
		static std::string name("Camera3D");
		return name;
	}
}
