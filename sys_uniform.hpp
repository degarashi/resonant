#pragma once
#include "spinner/optional.hpp"
#include "spinner/matrix.hpp"
#include "spinner/misc.hpp"
#include "resonant/handle.hpp"
#include "spinner/rflag.hpp"
#include "spinner/size.hpp"

namespace rs {
	class UnifBase {
		private:
			enum _Dummy { dummy };
		public:
			using value_t = uint32_t;
	};
	struct EUnif : UnifBase {
		struct Texture { enum {
			Diffuse,
			_Num
		};};
		const static std::string cs_word[];
		const static std::string& Get(value_t t);
	};
	// Uniform変数: 名前だけ定義
	struct EUnif3D : UnifBase {
		struct Texture { enum {
			Specular,
			Normal,
			_Num
		};};
		const static std::string cs_word[];
		const static std::string& Get(value_t t);
	};
	struct SysUnif : UnifBase {
		struct Screen { enum {
			size,
			_Num
		};};
		const static std::string cs_word[];
		const static std::string& Get(value_t t);
	};
	struct SysUnif2D : SysUnif {
		struct Matrix { enum {
			Transform,
			_Num
		};};
		const static std::string cs_word[];
		const static std::string& Get(value_t t);
	};
	// SystemUnif3Dが値の管理を行う
	struct SysUnif3D : SysUnif {
		struct Matrix { enum {
			Transform,
			TransformInv,
			Proj,
			ProjInv,
			View,
			ViewInv,
			ViewProj,
			ViewProjInv,
			World,
			WorldInv,
			EyePos,
			EyeDir,
			_Num
		};};
		const static std::string cs_word[];
		const static std::string& Get(value_t t);
	};

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
				((ViewRotation)(float)) \
				((Transform)(spn::Mat32)(ViewOffset)(ViewScale)(ViewRotation))
			RFLAG_S(SystemUniform2D, SEQ_SYSUNI2D)
		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI2D)
			#undef SEQ_SYSUNI2D

			void setViewOffset(const spn::Vec2& ofs);
			void setViewScale(const spn::Vec2& s);
			void setViewRotation(float deg);
			void outputUniforms(GLEffect& glx, bool bBase) const;
	};
	//! システムuniform変数をセットする(3D)
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
	class SystemUniform3D :
		public spn::CheckAlign<16, SystemUniform3D>,
		virtual public SystemUniformBase
	{
		private:
			#define SEQ_SYSUNI3D \
				((World)(spn::AMat44)) \
				((WorldInv)(spn::AMat44)(World)) \
				((Camera)(HLCamF))\
				((ViewInv)(spn::AMat44)(Camera))\
				((ProjInv)(spn::AMat44)(Camera))\
				((Transform)(spn::AMat44)(Camera)(World))\
				((TransformInv)(spn::AMat44)(Transform))
			RFLAG_S(SystemUniform3D, SEQ_SYSUNI3D)
			HLCamF		_hlCam;

		public:
			RFLAG_GETMETHOD_S(SEQ_SYSUNI3D)
			#undef SEQ_SYSUNI3D

			SystemUniform3D();
			void setCamera(HCam hCam);
			void setWorldMat(const spn::Mat43& m);
			void outputUniforms(GLEffect& glx, bool bBase) const;
	};
}

