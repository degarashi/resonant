#pragma once
#include "spinner/pose.hpp"

namespace rs {
	/*! 姿勢の保持はPose3Dクラスが行い，カメラ固有の変数だけを持つ */
	class CamData : public spn::Pose3D, public spn::CheckAlign<16, CamData> {
		GAP_MATRIX_DEF(multable, _matV, 3,4,
					   (((float, _fov)))
					   (((float, _aspect)))
					   (((float, _nearZ)))
					   (((float, _farZ)))
		)
		mutable spn::AMat44	_matVP, _matP, _matVPInv;
		// mutable VFrustum _vfrus;

	};
}
