#include "camera.hpp"
#include "glx.hpp"
#include "sys_uniform.hpp"

namespace rs {
	using spn::AMat43; using spn::AMat44;
	using spn::Vec2; using spn::Vec3;
	using spn::AVec4;
	using spn::Quat; using spn::Plane;
	using boom::geo3d::Frustum;
	uint32_t Camera3D::_refresh(spn::AMat44& m, Transform*) const {
		m = getWorld() * getViewProj();
		return 0;
	}
	uint32_t Camera3D::_refresh(spn::AMat44& m, TransformInv*) const {
		getTransform().inversion(m);
		return 0;
	}
	uint32_t Camera3D::_refresh(spn::AMat44& m, WorldInv*) const {
		getWorld().inversion(m);
		return 0;
	}
	uint32_t Camera3D::_refresh(uint32_t& acc, Accum*) const {
		// キャッシュ更新
		getPose();
		getFov();
		getAspect();
		getNearZ();
		getFarZ();
		getWorld();
		// カウンタをインクリメント
		++acc;
		return 0;
	}
	uint32_t Camera3D::_refresh(AMat43& m, View*) const {
		auto& ps = getPose();
		// 回転を一端quatに変換
		const Quat& q = ps.getRot();
		m.identity();
		m.getRow(3) = -ps.getOffset();

		Quat tq = q.inverse();
		m *= tq.asMat33();
		return 0;
	}
	uint32_t Camera3D::_refresh(AMat44& m, Proj*) const {
		m = AMat44::PerspectiveFovLH(getFov(), getAspect(), getNearZ(), getFarZ());
		return 0;
	}
	uint32_t Camera3D::_refresh(AMat44& m, ViewProj*) const {
		m = getView().convertA44() * getProj();
		return 0;
	}
	uint32_t Camera3D::_refresh(AMat44& m, ViewProjInv*) const {
		getViewProj().inversion(m);
		return 0;
	}
	uint32_t Camera3D::_refresh(Frustum& vf, VFrustum*) const {
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
		return 0;
	}

	Camera3D::Camera3D() {
		refPose().setAll(Vec3(0,0,-10), Quat(0,0,0,1), Vec3(1,1,1));
		setAccum(1);

		setNearZ(1.f);
		setFarZ(1e5f);
		setFov(spn::DegF(90.0f));
		setAspect(1.4f / 1.f);
		setWorld(spn::AMat44(1, spn::AMat44::TagDiagonal));
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

	void Camera3D::outputUniforms(GLEffect& glx) const {
		#define DEF_SETUNIF(name) \
			if(auto idv = glx.getUnifId(sysunif3d::matrix::name)) \
				glx.setUniform(*idv, get##name(), true);
		DEF_SETUNIF(World)
		DEF_SETUNIF(WorldInv)
		DEF_SETUNIF(View)
		DEF_SETUNIF(Proj)
		DEF_SETUNIF(ViewProj)
		DEF_SETUNIF(ViewProjInv)
		DEF_SETUNIF(Transform)
		DEF_SETUNIF(TransformInv)
		#undef DEF_SETUNIF

		auto& ps = getPose();
		if(auto idv = glx.getUnifId(sysunif3d::matrix::EyePos))
			glx.setUniform(*idv, ps.getOffset(), true);
		if(auto idv = glx.getUnifId(sysunif3d::matrix::EyeDir))
			glx.setUniform(*idv, ps.getRot().getZAxis(), true);
	}
}

