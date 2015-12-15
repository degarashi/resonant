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
			void moveFrom(SystemUniform& prev);
	};
	class SystemUniform3D : public spn::CheckAlign<16, SystemUniform3D> {
		private:
			struct Camera;
			struct Getter : spn::RFlag_Getter<uint32_t> {
				using RFlag_Getter::operator();
				counter_t operator()(const HLCamF& c, Camera*) const;
			};
			using Transform_t = spn::AcCheck<spn::AMat44, Getter>;
			#define SEQ_SYSUNI3D \
				((World)(spn::AMat44)) \
				((WorldInv)(spn::AMat44)(World)) \
				((Camera)(HLCamF)) \
				((Transform)(Transform_t)(World)(Camera)) \
				((TransformInv)(spn::AMat44)(Transform))
			RFLAG_S(SystemUniform3D, SEQ_SYSUNI3D)
			RFLAG_SETMETHOD(Transform)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI3D)
			RFLAG_REFMETHOD_S(SEQ_SYSUNI3D)
			RFLAG_SETMETHOD_S(SEQ_SYSUNI3D)
			#undef SEQ_SYSUNI3D

			SystemUniform3D();
			void outputUniforms(IEffect& glx) const;
			void moveFrom(SystemUniform3D& prev);
	};
	//! システムuniform変数をセットする(2D)
	/*! 変数リスト:
		mat3 sys_mTrans2D;		// scale * rotation * offset
	*/
	class SystemUniform2D {
		private:
			struct Camera;
			struct Getter : spn::RFlag_Getter<uint32_t> {
				using RFlag_Getter::operator();
				counter_t operator()(const HLCam2DF& c, Camera*) const;
			};
			using Transform_t = spn::AcCheck<spn::Mat33, Getter>;
			#define SEQ_SYSUNI2D \
				((World)(spn::Mat33)) \
				((WorldInv)(spn::Mat33)(World)) \
				((Camera)(HLCam2DF)) \
				((Transform)(Transform_t)(World)(Camera)) \
				((TransformInv)(spn::Mat33)(Transform))
			RFLAG_S(SystemUniform2D, SEQ_SYSUNI2D)
			RFLAG_SETMETHOD(Transform)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI2D)
			RFLAG_REFMETHOD_S(SEQ_SYSUNI2D)
			RFLAG_SETMETHOD_S(SEQ_SYSUNI2D)
			#undef SEQ_SYSUNI2D

			SystemUniform2D();
			void outputUniforms(IEffect& glx) const;
			void moveFrom(SystemUniform2D& prev);
	};
}
