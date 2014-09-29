#pragma once
#include "spinner/pose.hpp"
#include "spinner/plane.hpp"
#include "spinner/quat.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/alignedalloc.hpp"
#include "boomstick/geom3D.hpp"
#include "handle.hpp"

namespace rs {
	using spn::AMat43; using spn::AMat44;
	using spn::AVec3; using spn::AVec4;
	using spn::Vec2; using spn::Vec3;
	using spn::Pose3D;
	using spn::Plane;
	using spn::Quat; using spn::AQuat;
	using boom::geo3d::Frustum;

	/*! 姿勢の保持はPose3Dクラスが行い，カメラ固有の変数だけを持つ */
	class CamData : public Pose3D, public spn::CheckAlign<16, CamData> {
		GAP_MATRIX_DEF(mutable, _matV, 4,3,
					   (((spn::RadF, _fov)))
					   (((float, _aspect)))
					   (((float, _nearZ)))
					   (((float, _farZ)))
		)
		// 変換キャッシュ
		mutable AMat44	_matVP, _matP, _matVPInv;
		mutable Frustum _vfrus;
		// リフレッシュAccum値 (更新した時点の値)
		mutable uint32_t 	_accum,
							_acMat,
							_acMatInv,
							_acVFrus;
		bool _checkAC(uint32_t& acDst) const;
		void _calcMatrices() const;				// View,Proj,(Both)
		void _calcVPInv() const;				// InverseMat(VP)
		void _calcFrustum() const;

		public:
			using Vec3x2 = std::pair<Vec3,Vec3>;
			CamData();
			CamData(const CamData& c);

			//! accumulation counter
			uint32_t getAccum() const;

			// アスペクト，FOV，Z平面
			void setAspect(float ap);
			void setFOV(spn::RadF fv);
			void setZPlane(float n, float f);
			void setNearDist(float n);
			void setFarDist(float f);
			// 各カメラ行列参照
			const AMat44& getViewProjMatrix() const;
			const AMat44& getProjMatrix() const;
			const AMat43& getViewMatrix() const;
			const AMat44& getViewProjInv() const;
			// 各パラメータ取得
			spn::RadF getFOV() const;
			float getAspect() const;
			float getFarDist() const;
			float getNearDist() const;
			Plane getNearPlane() const;
			const Frustum& getFrustum() const;
			Frustum getNearFrustum() const;
			//! ビューポート座標をワールド座標系に変換
			Vec3 unproject(const Vec3& vsPos) const;		// 単体
			Vec3x2 unprojectVec(const Vec2& vsPos) const;	// (Near,Far)
			//! ビューポート座標からワールド座標(FarPlane位置)を取得
			Vec3 vp2wp(const Vec3& vp) const;

			Pose3D getPose() const;
			void setPose(const Pose3D& ps);
	};
	#define mgr_cam (::rs::CameraMgr::_ref())
	class CameraMgr : public spn::ResMgrA<CamData, CameraMgr, spn::Alloc16> {};
}
