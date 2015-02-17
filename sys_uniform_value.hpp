#pragma once
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
}
