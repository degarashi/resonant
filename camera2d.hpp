#pragma once
#include "spinner/rflag.hpp"
#include "spinner/pose.hpp"
#include "spinner/structure/angle.hpp"
#include "spinner/structure/wrapper.hpp"
#include "glx_id.hpp"
#include "handle.hpp"

namespace rs {
	/*! 2Dゲームの他にミニマップ表示などで使用
		姿勢の保持はPose2Dクラスが行い，カメラ固有の変数だけを持つ */
	class Camera2D : public spn::CheckAlign<16,Camera2D> {
		private:
			struct Pose;
			struct Getter : spn::RFlag_Getter<uint32_t> {
				using RFlag_Getter::operator ();
				counter_t operator()(const spn::Pose2D& pose, Pose*, const Camera2D&) const {
					return pose.getAccum();
				}
			};
			using View_t = spn::AcCheck<spn::Mat33, Getter>;
			using Accum_t = spn::AcCheck<spn::Wrapper<uint32_t>, Getter>;
			#define SEQ_CAMERA2D \
				((Pose)(spn::Pose2D)) \
				((View)(View_t)(Pose)) \
				((ViewInv)(spn::Mat33)(View)) \
				((AspectRatio)(float)) \
				((Proj)(spn::Mat33)(AspectRatio)) \
				((ViewProj)(spn::Mat33)(View)(Proj)) \
				((ViewProjInv)(spn::Mat33)(ViewProj)) \
				((Accum)(Accum_t)(Pose)(AspectRatio))
			RFLAG_S(Camera2D, SEQ_CAMERA2D)
			RFLAG_SETMETHOD(Accum)
		public:
			RFLAG_GETMETHOD_S(SEQ_CAMERA2D)
			RFLAG_SETMETHOD_S(SEQ_CAMERA2D)
			RFLAG_REFMETHOD(Pose)
			#undef SEQ_CAMERA2D

			Camera2D();
			//! Projection後の座標をワールド座標へ変換
			spn::Vec2 vp2w(const spn::Vec2& pos) const;
			//! View座標をワールド座標へ変換
			spn::Vec2 v2w(const spn::Vec2& pos) const;
	};
	#define mgr_cam2d (::rs::Camera2DMgr::_ref())
	class Camera2DMgr : public spn::ResMgrA<Camera2D, Camera2DMgr, spn::Alloc16> {
		public:
			const std::string& getResourceName(spn::SHandle sh) const override;
	};
}
#include "luaimport.hpp"
DEF_LUAIMPORT(rs::Camera2D)
