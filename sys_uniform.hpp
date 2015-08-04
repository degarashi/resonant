#pragma once
#include "spinner/optional.hpp"
#include "spinner/matrix.hpp"
#include "spinner/misc.hpp"
#include "handle.hpp"
#include "spinner/rflag.hpp"
#include "spinner/size.hpp"
#include "sys_uniform_value.hpp"

namespace rs {
	struct IEffect;
	//! (3D/2D共通)
	/*! 予め変数名がsys_*** の形で決められていて, 存在すれば計算&設定される
		固定変数リスト:
		vec4 sys_screen;		// x=ScreenSizeX, y=ScreenSizeY, z=1DotSizeX, w=1DotSizeY
	*/
	class SystemUniform {
		private:
			spn::Size	_screenSize;
		public:
			const spn::Size& getScreenSize() const;
			void setScreenSize(const spn::Size& s);
			void outputUniforms(IEffect& glx) const;
	};
	class SystemUniform3D : public spn::CheckAlign<16, SystemUniform3D> {
		private:
			#define SEQ_SYSUNI3D \
				((World)(spn::AMat44)) \
				((WorldInv)(spn::AMat44)(World)) \
				((Camera)(HLCamF)) \
				((CameraAc)(uint32_t)(Camera)) \
				((Transform)(spn::AcWrapper<spn::AMat44>)(World)(CameraAc)) \
				((TransformInv)(spn::AMat44)(Transform))
			RFLAG_S(SystemUniform3D, SEQ_SYSUNI3D)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI3D)
			RFLAG_REFMETHOD_S(SEQ_SYSUNI3D)
			RFLAG_SETMETHOD_S(SEQ_SYSUNI3D)
			#undef SEQ_SYSUNI3D

			SystemUniform3D();
			void outputUniforms(IEffect& glx) const;
	};
	//! システムuniform変数をセットする(2D)
	/*! 変数リスト:
		mat3 sys_mTrans2D;		// scale * rotation * offset
	*/
	class SystemUniform2D {
		private:
			#define SEQ_SYSUNI2D \
				((World)(spn::Mat33)) \
				((WorldInv)(spn::Mat33)(World)) \
				((Camera)(HLCam2DF)) \
				((CameraAc)(uint32_t)(Camera)) \
				((Transform)(spn::AcWrapper<spn::Mat33>)(World)(CameraAc)) \
				((TransformInv)(spn::Mat33)(Transform))
			RFLAG_S(SystemUniform2D, SEQ_SYSUNI2D)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI2D)
			RFLAG_SETMETHOD_S(SEQ_SYSUNI2D)
			#undef SEQ_SYSUNI2D

			SystemUniform2D();
			void outputUniforms(IEffect& glx) const;
	};
}
