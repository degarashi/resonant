#include "camera.hpp"

namespace rs {
	bool CamData::_checkAC(uint32_t& acDst) const {
		uint32_t ac = getAccum();
		if(acDst != ac) {
			acDst = ac;
			return true;
		}
		return false;
	}

	void CamData::_calcMatrices() const {
		if(_checkAC(_acMat)) {
			// 回転を一端quatに変換
			const Quat& q = getRot();
			_matV.identity();
			_matV.getRow(3) = -getOffset();

			Quat tq = q.inverse();
			_matV *= tq.asMat33();

			_matP = AMat44::PerspectiveFovLH(_fov, _aspect, _nearZ, _farZ);
			_matVP = _matV.convertA44() * _matP;
		}
	}
	void CamData::_calcVPInv() const {
		if(_checkAC(_acMatInv)) {
			_calcMatrices();
			_matVP.inversion(_matVPInv);
		}
	}
	void CamData::_calcFrustum() const {
		if(_checkAC(_acVFrus)) {
			// UP軸とZ軸を算出
			AMat44 mat;
			getViewMatrix().convertA44().inversion(mat);
			mat.getRow(3) = AVec4(0,0,0,1);
			AVec4 zAxis(0,0,1,0),
				yAxis(0,1,0,0);
			zAxis *= mat;
			yAxis *= mat;

			_vfrus = Frustum(getOffset(), zAxis.asVec3(), yAxis.asVec3(), _fov, _farZ, _aspect);
		}
	}

	CamData::CamData():
		Pose3D(Vec3(0,0,-10), Quat(0,0,0,1), Vec3(1,1,1)),
		_accum(1), _acMat(0), _acMatInv(0), _acVFrus(0)
	{
		_nearZ = 1.f;
		_farZ = 1e5f;
		_fov = spn::DegF(90.0f);
		_aspect = 1.4f / 1.f;
	}
	CamData::CamData(const CamData& c): Pose3D(static_cast<const Pose3D&>(c)), _matV() {
		uintptr_t ths = reinterpret_cast<uintptr_t>(this) + sizeof(Pose3D),
				src = reinterpret_cast<uintptr_t>(&c) + sizeof(Pose3D);
		std::memcpy(reinterpret_cast<void*>(ths), reinterpret_cast<const void*>(src), sizeof(CamData)-sizeof(Pose3D));
	}
	uint32_t CamData::getAccum() const {
		return _accum + ((Pose3D*)this)->getAccum();
	}

	void CamData::setAspect(float ap) {
		_aspect = ap;
		++_accum;
	}
	void CamData::setFOV(spn::RadF fv) {
		_fov = fv;
		++_accum;
	}
	void CamData::setZPlane(float n, float f) {
		_nearZ = n;
		_farZ = f;
		++_accum;
	}
	void CamData::setNearDist(float n) {
		_nearZ = n;
		++_accum;
	}
	void CamData::setFarDist(float f) {
		_farZ = f;
		++_accum;
	}

	const AMat44& CamData::getViewProjMatrix() const {
		_calcMatrices();
		return _matVP;
	}
	const AMat43& CamData::getViewMatrix() const {
		_calcMatrices();
		return _matV;
	}
	const AMat44& CamData::getProjMatrix() const {
		_calcMatrices();
		return _matP;
	}
	const AMat44& CamData::getViewProjInv() const {
		_calcVPInv();
		return _matVPInv;
	}

	spn::RadF CamData::getFOV() const {
		return _fov;
	}
	float CamData::getAspect() const {
		return _aspect;
	}
	float CamData::getFarDist() const {
		return _farZ;
	}
	float CamData::getNearDist() const {
		return _nearZ;
	}
	const Frustum& CamData::getFrustum() const {
		_calcFrustum();
		return _vfrus;
	}
	Vec3 CamData::unproject(const Vec3& vsPos) const {
		const AMat44& mI = getViewProjInv();
		return (vsPos.asVec4(1) * mI).asVec3Coord();
	}
	CamData::Vec3x2 CamData::unprojectVec(const Vec2& vsPos) const {
		const AMat44& mI = getViewProjInv();
		return Vec3x2(
			(AVec4(vsPos.x, vsPos.y, 0, 1) * mI).asVec3Coord(),
			(AVec4(vsPos.x, vsPos.y, 1, 1) * mI).asVec3Coord()
		);
	}
	Vec3 CamData::vp2wp(const Vec3& vp) const {
		return (vp.asVec4(1) * getViewProjInv()).asVec3Coord();
	}
	Pose3D CamData::getPose() const {
		return Pose3D(getOffset(), getRot(), AVec3(1,1,1));
	}
	void CamData::setPose(const Pose3D& ps) {
		setOffset(ps.getOffset());
		setRot(ps.getRot());
		// (スケールは適用しない)
	}
	Plane CamData::getNearPlane() const {
		Vec3 dir = getDir();
		return Plane::FromPtDir(getOffset() + dir*getNearDist(), dir);
	}
	// カメラの変換手順としては offset -> rotation だがPose3Dの変換は rotation -> offsetなので注意！
	Frustum CamData::getNearFrustum() const {
		Frustum fr;
		float t = std::tan(_fov->get()/2);
		fr.setScale({t*_aspect, t, getNearDist()*8});
		fr.setRot(getRot());
		fr.setOffset(getOffset());
		return fr;
	}
}
