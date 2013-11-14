#pragma once

// OpenGL関数プロトタイプは自分で定義するのでマクロを解除しておく
#undef GL_GLEXT_PROTOTYPES
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

#include "spinner/error.hpp"
// OpenGLに関するアサート集
#define GLEC_Base(act, chk, ...)			::spn::EChk_base(act, chk, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define GLEC_Base0(act, chk)				::spn::EChk_base(act, chk, __FILE__, __PRETTY_FUNCTION__, __LINE__);
#define GLEC(act, ...)						GLEC_Base(AAct_##act<GLE_Error>(), GLError(), __VA_ARGS__)
#define GLEC_Chk(act)						GLEC_Base0(AAct_##act<GLE_Error>(), GLError())

#ifdef DEBUG
	#define GLEC_P(act, chk, ...)			GLEC(act, chk, __VA_ARGS__)
	#define GLEC_ChkP(act)					GLEC_Chk(act)
#else
	#define GLEC_P(act, chk, ...)			::spn::EChk_pass(act, chk, __VA_ARGS__)
	#define GLEC_ChkP(act)
#endif

namespace rs {
	//! OpenGLエラーIDとその詳細メッセージ
	struct GLError {
		const static std::pair<GLenum, const char*> ErrorList[];

		const char* errorDesc() const;
		void reset() const;
		const char* getAPIName() const;
		//! エラー値を出力しなくなるまでループする
		void resetError() const;
	};
	struct GLProgError {
		GLuint	_id;
		GLProgError(GLuint id);
		const char* errorDesc() const;
		void reset() const;
		const char* getAPIName() const;
	};
	struct GLShError {
		GLuint	_id;
		GLShError(GLuint id);
		const char* errorDesc() const;
		void reset() const;
		const char* getAPIName() const;
	};
	//! OpenGLに関する全般的なエラー
	struct GLE_Error : std::runtime_error {
		static const char* GetErrorName() { return "OpenGL Error"; }
		using runtime_error::runtime_error;
	};
}
