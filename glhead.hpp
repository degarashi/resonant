#pragma once

#ifdef ANDROID
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#else
	#include <GL/gl.h>
#endif

#if !defined(WIN32)
	#ifndef ANDROID
		#include <GL/glx.h>
		#undef Convex
		#include "glext.h"
		#include "glxext.h"
	#endif
#else
	#ifndef ANDROID
		#include "glext.h"
	#endif
#endif

#ifndef ANDROID
	#define GLDEFINE(name,type)		extern type name;
	#include "glfunc.inc"
	#undef GLDEFINE
#endif

#include <memory>
#include <vector>
#include <assert.h>

namespace rs {
	extern void LoadGLFunc();
	extern bool IsGLFuncLoaded();
	extern void SetSwapInterval(int n);

	struct IGLResource;
	using UPResource = std::unique_ptr<IGLResource>;
	class GLEffect;
	using UPEffect = std::unique_ptr<GLEffect>;
	class VDecl;
	using SPVDecl = std::shared_ptr<VDecl>;
	class TPStructR;
	class GLBuffer;
	using UPBuffer = std::unique_ptr<GLBuffer>;
	class GLVBuffer;
	using UPVBuffer = std::unique_ptr<GLVBuffer>;
	class GLIBuffer;
	using UPIBuffer = std::unique_ptr<GLIBuffer>;
	class IGLTexture;
	using UPTexture = std::unique_ptr<IGLTexture>;
	class GLProgram;
	using UPProg = std::unique_ptr<GLProgram>;
	class GLShader;
	using UPShader = std::unique_ptr<GLShader>;
	class GLFBuffer;
	using UPFBuffer = std::unique_ptr<GLFBuffer>;
	class GLRBuffer;
	using UPRBuffer = std::unique_ptr<GLRBuffer>;

	enum ShType : unsigned int {
		VERTEX, GEOMETRY, PIXEL,
		NUM_SHTYPE
	};
	//! シェーダーIDに対するOpenGL定数
	const static GLuint c_glShFlag[ShType::NUM_SHTYPE] = {
		GL_VERTEX_SHADER,
		#ifdef ANDROID
			0,
		#else
			GL_GEOMETRY_SHADER,
		#endif
		GL_FRAGMENT_SHADER
	};
}

#include "error.hpp"
// OpenGLに関するアサート集
#define GLEC_Base(act, ...)		EChk_base<GLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define GLEC_Base0(act)			EChk_base<GLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__);
#define GLEC(act, ...)			GLEC_Base(AAct_##act<GLE_Error>(), __VA_ARGS__)
#define GLECProg(act, id, ...)	GLEC_Base(AAct_##act<GLE_ProgramError, GLuint>(id), __VA_ARGS__)
#define GLECSh(act, id, ...)	GLEC_Base(AAct_##act<GLE_ShaderError, GLuint>(id), __VA_ARGS__)
#define GLECParam(act, name, ...)	GLEC_Base(AAct_##act<GLE_ParamNotFound, const char*>(name), __VA_ARGS__)
#define GLECArg(act, shname, argname, ...)	GLEC_Base(AAct_##act<GLE_InvalidArgument, const char*, const char*>(shname, argname), __VA_ARGS__)
#define GLECLog(act, ...)		GLEC_Base(AAct_##act<GLE_LogicalError>(), __VA_ARGS__)

#define GLEC_Chk(act)			GLEC_Base0(AAct_##act<GLE_Error>())
#ifdef DEBUG
	#define GLEC_P(act, ...)	GLEC(act, __VA_ARGS__)
	#define GLEC_ChkP(act)		GLEC_Chk(act)
#else
	#define GLEC_P(act, ...)	EChk_pass(__VA_ARGS__)
	#define GLEC_ChkP(act)
#endif
namespace rs {
	//! OpenGLエラーIDとその詳細メッセージ
	struct GLError {
		const static std::pair<GLenum, const char*> ErrorList[];

		static std::string s_tmp;
		static const char* ErrorDesc();
		static void Reset();
		static const char* GetAPIName();
		//! エラー値を出力しなくなるまでループする
		static void ResetError();
	};
	//! OpenGLに関する全般的なエラー
	struct GLE_Error : std::runtime_error {
		using runtime_error::runtime_error;
	};
}
