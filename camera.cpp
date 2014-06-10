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
		_fov = spn::DEGtoRAD(90.0f);
		_aspect = 1.4f / 1.f;
	}
	CamData::CamData(const CamData& c): Pose3D(static_cast<const Pose3D&>(c)), _matV() {
		uintptr_t ths = reinterpret_cast<uintptr_t>(this) + sizeof(Pose3D),
				src = reinterpret_cast<uintptr_t>(&c) + sizeof(Pose3D);
		std::memcpy(reinterpret_cast<void*>(ths), reinterpret_cast<const void*>(src), sizeof(CamData)-sizeof(Pose3D));
	}
	void CamData::moveFwd2D(float speed) {
		Vec3 vZ = getDir();
		vZ.y = 0;
		vZ.normalize();
		addOffset(vZ * speed);
	}
	void CamData::moveSide2D(float speed) {
		Vec3 vX = getRight();
		vX.y = 0;
		vX.normalize();
		addOffset(vX * speed);
	}
	void CamData::moveFwd3D(float speed) {
		addOffset(getDir() * speed);
	}
	void CamData::moveSide3D(float speed) {
		addOffset(getRight() * speed);
	}
	void CamData::turnAxis(const Vec3& axis, float rad) {
		Quat q = getRot();
		q.rotate(axis, rad);
		setRot(q);
	}
	void CamData::turnYPR(float yaw, float pitch, float roll) {
		Quat q = getRot();
		q >>= Quat::RotationYPR(yaw, pitch, roll);
		setRot(q);
	}
	void CamData::addRot(const Quat& q) {
		Quat q0 = getRot();
		q0 >>= q;
		setRot(q0);
	}
	bool CamData::lerpTurn(const Quat& q_tgt, float t) {
		Quat q = getRot();
		q.slerp(q_tgt, t);
		return q.distance(q_tgt) < TURN_THRESHOLD;
	}
	uint32_t CamData::getAccum() const {
		return _accum + ((Pose3D*)this)->getAccum();
	}

	void CamData::setAspect(float ap) {
		_aspect = ap;
		++_accum;
	}
	void CamData::setFOV(float fv) {
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

	float CamData::getFOV() const {
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
	void CamData::unproject(Vec3& vdPos, const Vec3& vsPos) {
		const AMat44& mI = getViewProjInv();
		vdPos = AVec4(vsPos.asVec4(1) * mI).asVec3Coord();
	}
	void CamData::unprojectVec(Vec3& vdBegin, Vec3& vdEnd, const Vec2& vsPos) {
		const AMat44& mI = getViewProjInv();
		vdBegin = AVec4(AVec4(vsPos.x, vsPos.y, 0, 1) * mI).asVec3Coord();
		vdEnd = AVec4(AVec4(vsPos.x, vsPos.y, 1, 1) * mI).asVec3Coord();
	}
	Vec3 CamData::vp2wp(float x, float y, float z) {
		AVec4 v(x,y,z,1);
		const AMat44& m = getViewProjMatrix();
		AMat44 im;
		m.inversion(im);
		v *= im;

		float inv = 1.0f/v.w;
		return Vec3(v.x*inv, v.y*inv, v.z*inv);
	}
	Pose3D CamData::getPose() const {
		return Pose3D(getOffset(), getRot(), AVec3(1,1,1));
	}
	void CamData::setPose(const Pose3D& ps) {
		setOffset(ps.getOffset());
		setRot(ps.getRot());
		// (スケールは適用しない)
	}
	void CamData::adjustNoRoll() {
		// X軸のY値が0になればいい

		// 回転を一旦行列に直して軸を再計算
		const auto& q = getRot();
		auto rm = q.asMat33();
		// Zはそのままに，X軸のY値を0にしてY軸を復元
		Vec3 zA = rm.getRow(2),
			xA = rm.getRow(0);
		xA.y = 0;
		if(xA.len_sq() < 1e-5f) {
			// Xが真上か真下を向いている
			float ang;
			if(rm.ma[0][1] > 0) {
				// 真上 = Z軸周りに右へ90度回転
				ang = spn::DEGtoRAD(90);
			} else {
				// 真下 = 左へ90度
				ang = spn::DEGtoRAD(-90);
			}
			setRot(AQuat::RotationZ(ang) * q);
		} else {
			xA.normalize();
			Vec3 yA = zA % xA;
			setRot(Quat::FromAxis(xA, yA, zA));
		}
	}
	Plane CamData::getNearPlane() const {
		Vec3 dir = getDir();
		return Plane::FromPtDir(getOffset() + dir*getNearDist(), dir);
	}
	// カメラの変換手順としては offset -> rotation だがPose3Dの変換は rotation -> offsetなので注意！
	Frustum CamData::getNearFrustum() const {
		Frustum fr;
		float t = std::tan(_fov/2);
		fr.setScale({t*_aspect, t, getNearDist()*8});
		fr.setRot(getRot());
		fr.setOffset(getOffset());
		return fr;
	}
}
