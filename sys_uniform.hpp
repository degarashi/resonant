#pragma once
#include "spinner/optional.hpp"
#include "spinner/matrix.hpp"
#include "spinner/misc.hpp"
#include "handle.hpp"
#include "spinner/rflag.hpp"
#include "spinner/size.hpp"
#include "glx_id.hpp"

namespace rs {
	namespace unif {
		namespace texture {
			extern const IdValue Diffuse;		// "sys_texDiffuse"
		}
	}
	namespace unif2d {
		using namespace unif;
		extern const IdValue	Depth,			// "sys_fDepth"
								Alpha;			// "sys_fAlpha"
	}
	namespace unif3d {
		using namespace unif;
		namespace texture {
			using namespace unif::texture;
			extern const IdValue	Specular,	// "sys_texSpecular"
									Normal,		// "sys_texNormal"
									Emissive;	// "sys_texEmissive"
		}
	}
	namespace sysunif {
		namespace screen {
			extern const IdValue Size;			// "sys_vScreenSize"
		}
	}
	namespace sysunif2d {
		using namespace sysunif;
		namespace matrix {
			extern const IdValue Transform;		// "sys_mTrans2D"
		}
	}
	namespace sysunif3d {
		using namespace sysunif;
		namespace matrix {
			extern const IdValue	Transform,		// "sys_mTrans"
									TransformInv,	// "sys_mTransInv"
									Proj,			// "sys_mProj"
									ProjInv,		// "sys_mProjInv"
									View,			// "sys_mView"
									ViewInv,		// "sys_mViewInv"
									ViewProj,		// "sys_mProj"
									ViewProjInv,	// "sys_mProjInv"
									World,			// "sys_mWorld"
									WorldInv,		// "sys_mWorldInv"
									EyePos,			// "sys_vEyePos"
									EyeDir;			// "sys_vEyeDir"
		}
	}

	//! (3D/2D共通)
	/*! 予め変数名がsys_*** の形で決められていて, 存在すれば計算&設定される
		固定変数リスト:
		vec4 sys_screen;		// x=ScreenSizeX, y=ScreenSizeY, z=1DotSizeX, w=1DotSizeY
	*/
	class SystemUniformBase {
		private:
			spn::Size	_screenSize;
		public:
			const spn::Size& getScreenSize() const;
			void setScreenSize(const spn::Size& s);
			void outputUniforms(GLEffect& glx) const;
	};
	//! システムuniform変数をセットする(2D)
	/*! 変数リスト:
		mat3 sys_mTrans2D;		// scale * rotation * offset
	*/
	class SystemUniform2D : virtual public SystemUniformBase {
		private:
			#define SEQ_SYSUNI2D \
				((ViewOffset)(spn::Vec2)) \
				((ViewScale)(spn::Vec2)) \
				((ViewRotation)(spn::DegF)) \
				((Transform2D)(spn::Mat32)(ViewOffset)(ViewScale)(ViewRotation))
			RFLAG_S(SystemUniform2D, SEQ_SYSUNI2D)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI2D)
			RFLAG_SETMETHOD_S(SEQ_SYSUNI2D)
			#undef SEQ_SYSUNI2D

			void setTransform2D(const spn::Mat32& m);
			void outputUniforms(GLEffect& glx, bool bBase) const;
	};
}

