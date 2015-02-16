#pragma once
#include "spinner/pose.hpp"
#include "spinner/plane.hpp"
#include "spinner/quat.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/alignedalloc.hpp"
#include "spinner/rflag.hpp"
#include "boomstick/geom3D.hpp"
#include "handle.hpp"

namespace rs {
	class GLEffect;
	/*! 姿勢の保持はPose3Dクラスが行い，カメラ固有の変数だけを持つ */
	class Camera3D : public spn::CheckAlign<16, Camera3D> {
		private:
			#define SEQ_CAMERA3D \
				((Pose)(spn::Pose3D)) \
				((View)(spn::AMat43)(Pose)) \
				((Fov)(spn::RadF)) \
				((Aspect)(float)) \
				((NearZ)(float)) \
				((FarZ)(float)) \
				((Proj)(spn::AMat44)(Fov)(Aspect)(NearZ)(FarZ)) \
				((ViewProj)(spn::AMat44)(View)(Proj)) \
				((ViewProjInv)(spn::AMat44)(ViewProj)) \
				((VFrustum)(boom::geo3d::Frustum)(View)(Fov)(Aspect)(NearZ)(FarZ)) \
				((Accum)(uint32_t)(Pose)(Fov)(Aspect)(NearZ)(FarZ))
			RFLAG_S(Camera3D, SEQ_CAMERA3D)
			RFLAG_SETMETHOD(Accum)
		public:
			using Vec3x2 = std::pair<spn::Vec3, spn::Vec3>;
			Camera3D();

			RFLAG_GETMETHOD_S(SEQ_CAMERA3D)
			RFLAG_SETMETHOD_S(SEQ_CAMERA3D)
			RFLAG_REFMETHOD_S(SEQ_CAMERA3D)
			#undef SEQ_CAMERA3D

			void setZPlane(float n, float f);

			spn::Plane getNearPlane() const;
			boom::geo3d::Frustum getNearFrustum() const;
			//! ビューポート座標をワールド座標系に変換
			spn::Vec3 unproject(const spn::Vec3& vsPos) const;		// 単体
			Vec3x2 unprojectVec(const spn::Vec2& vsPos) const;	// (Near,Far)
			//! ビューポート座標からワールド座標(FarPlane位置)を取得
			spn::Vec3 vp2wp(const spn::Vec3& vp) const;

			//! シェーダーにカメラ変数をUniformで渡す
			/*! 変数リスト:
				mat4 sys_mTrans,		// mWorld * mView * mProj
					sys_mTransInv;		// inverse(mTrans)
				mat4 sys_mView,
					sys_mViewInv;		// inverse(mView)
				mat4 sys_mProj,
					sys_mProjInv;		// inverse(mProj)
				mat4 sys_mViewProj,		// mView * mProj
					sys_mViewProjInv;	// inverse(mViewProj)
				mat4 sys_mWorld,
					sys_mWorldInv;		// inverse(mWorld)
				vec3 sys_vEyePos,
					sys_vEyeDir;
			*/
			void outputUniforms(GLEffect& glx, const spn::AMat43& wmat) const;
	};
	#define mgr_cam (::rs::Camera3DMgr::_ref())
	class Camera3DMgr : public spn::ResMgrA<Camera3D, Camera3DMgr, spn::Alloc16> {};
}
