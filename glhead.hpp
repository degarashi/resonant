#pragma once
#include "sdlwrap.hpp"

// OpenGL関数プロトタイプは自分で定義するのでマクロを解除しておく
#undef GL_GLEXT_PROTOTYPES
#ifdef ANDROID
	#include <GLES2/gl2.h>
	#include <GLES2/gl2ext.h>
#else
	#include <GL/gl.h>
#endif

#if !defined(WIN32) && !defined(ANDROID)
	#include <GL/glx.h>
	#undef Convex
	#include "glext.h"
	#include "glxext.h"
#endif
#if defined(WIN32)
	#include <GL/glcorearb.h>
	#include "glext.h"
#endif

#ifndef ANDROID
	#include "glext.h"
#endif

namespace rs {
	struct IGL {
		#define GLDEFINE(...)
		#define DEF_GLMETHOD(ret_type, name, args, argnames) \
			virtual ret_type name(BOOST_PP_SEQ_ENUM(args)) = 0;

		#ifdef ANDROID
			#include "android_gl.inc"
		#elif defined(WIN32)
			#include "mingw_gl.inc"
		#else
			#include "linux_gl.inc"
		#endif

		#undef DEF_GLMETHOD
		#undef GLDEFINE
		virtual void setSwapInterval(int n) = 0;
		virtual void stencilFuncFront(int func, int ref, int mask) = 0;
		virtual void stencilFuncBack(int func, int ref, int mask) = 0;
		virtual void stencilOpFront(int sfail, int dpfail, int dppass) = 0;
		virtual void stencilOpBack(int sfail, int dpfail, int dppass) = 0;
		virtual void stencilMaskFront(int mask) = 0;
		virtual void stencilMaskBack(int mask) = 0;
	};
	//! 直でOpenGL関数を呼ぶ
	struct IGL_Draw : IGL {
		#define GLDEFINE(...)
		#define DEF_GLMETHOD(ret_type, name, args, argnames) \
			virtual ret_type name(BOOST_PP_SEQ_ENUM(args)) override;

		#ifdef ANDROID
			#include "android_gl.inc"
		#elif defined(WIN32)
			#include "mingw_gl.inc"
		#else
			#include "linux_gl.inc"
		#endif

		void setSwapInterval(int n) override;
		void stencilFuncFront(int func, int ref, int mask)  override;
		void stencilFuncBack(int func, int ref, int mask)  override;
		void stencilOpFront(int sfail, int dpfail, int dppass)  override;
		void stencilOpBack(int sfail, int dpfail, int dppass)  override;
		void stencilMaskFront(int mask)  override;
		void stencilMaskBack(int mask)  override;
	};
	//! DrawThreadにOpenGL関数呼び出しを委託
	struct IGL_OtherSingle : IGL {
		
		#ifdef ANDROID
			#include "android_gl.inc"
		#elif defined(WIN32)
			#include "mingw_gl.inc"
		#else
			#include "linux_gl.inc"
		#endif
		
		#undef DEF_GLMETHOD
		#undef GLDEFINE

		void setSwapInterval(int n) override;
		void stencilFuncFront(int func, int ref, int mask)  override;
		void stencilFuncBack(int func, int ref, int mask)  override;
		void stencilOpFront(int sfail, int dpfail, int dppass)  override;
		void stencilOpBack(int sfail, int dpfail, int dppass)  override;
		void stencilMaskFront(int mask)  override;
		void stencilMaskBack(int mask)  override;
	};
	extern TLS<IGL*>	tls_GL;
	#define GL	(*(*::rs::tls_GL))

	#define GLW	(::rs::GLWrap::_ref())
	class Handler;
	//! OpenGL APIラッパー
	class GLWrap : public spn::Singleton<GLWrap> {
		IGL_Draw			_ctxDraw;
		IGL_OtherSingle		_ctxSingle;
		bool				_bShare;
		Handler*			_drawHandler;

		// ---- Context共有データ ----
		using Shared = SpinLockP<void*>;
		Shared				_pShared;

		public:
			#define GLDEFINE(...)
			#define DEF_GLMETHOD(ret_type, name, args, argnames) \
				using t_##name = ret_type (*)(BOOST_PP_SEQ_ENUM(args)); \
				static t_##name name;

			#ifdef ANDROID
				#include "android_gl.inc"
			#elif defined(WIN32)
				#include "mingw_gl.inc"
			#else
				#include "linux_gl.inc"
			#endif

			#undef DEF_GLMETHOD
			#undef GLDEFINE
		public:
			GLWrap(bool bShareEnabled);
			void loadGLFunc();
			bool isGLFuncLoaded();
			void initializeMainThread();
			void initializeDrawThread(Handler& handler);
			void terminateDrawThread();

			Handler& getDrawHandler();
			Shared& refShared();
	};
	template <class T>
	struct GLSharedData {
		GLSharedData() {
			auto lk = _lock();
			*lk = new T();
		}
		~GLSharedData() {
			auto lk = _lock();
			delete reinterpret_cast<T*>(*lk);
		}
		decltype(GLW.refShared().lock().castAndMove<T*>()) _lock() {
			return GLW.refShared().lock().castAndMove<T*>();
		}
		decltype(GLW.refShared().lock().castAndMoveDeRef<T>()) lock() {
			return GLW.refShared().lock().castAndMoveDeRef<T>();
		}
	};
}

#include <memory>
#include <vector>
#include <assert.h>

namespace rs {
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
#define GLEC_Base(act, ...)				::spn::EChk_base(act, GLError(), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define GLEC_Base0(act)					::spn::EChk_base(act, GLError(), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define GLEC(act, func, ...)			GLEC_Base(AAct_##act<GLE_Error>(), [&](){GL.func(__VA_ARGS__);})
#define GLEC_Chk(act)					GLEC_Base0(AAct_##act<GLE_Error>());

#ifdef DEBUG
	#define GLEC_P(act, ...)			GLEC(act, __VA_ARGS__)
	#define GLEC_ChkP(act)				GLEC_Chk(act)
#else
    #define GLEC_P(act, func, ...)		::spn::EChk_pass([&](){GL.func(__VA_ARGS__);})
	#define GLEC_ChkP(act)
#endif

namespace rs {
	//! OpenGLエラーIDとその詳細メッセージ
	struct GLError {
		std::string _errMsg;
		const static std::pair<GLenum, const char*> ErrorList[];

		const char* errorDesc();
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
