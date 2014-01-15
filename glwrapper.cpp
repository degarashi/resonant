#include "glhead.hpp"
#include "event.hpp"

namespace rs {
	TLS<IGL*>	tls_GL;
	void IGL_Draw::stencilFuncFront(int func, int ref, int mask) {
		GLM::glStencilFuncSeparate(GL_FRONT, func, ref, mask);
	}
	void IGL_Draw::stencilFuncBack(int func, int ref, int mask) {
		GLM::glStencilFuncSeparate(GL_BACK, func, ref, mask);
	}
	void IGL_Draw::stencilOpFront(int sfail, int dpfail, int dppass) {
		GLM::glStencilOpSeparate(GL_FRONT, sfail, dpfail, dppass);
	}
	void IGL_Draw::stencilOpBack(int sfail, int dpfail, int dppass) {
		GLM::glStencilOpSeparate(GL_BACK, sfail, dpfail, dppass);
	}
	void IGL_Draw::stencilMaskFront(int mask) {
		GLM::glStencilMaskSeparate(GL_FRONT, mask);
	}
	void IGL_Draw::stencilMaskBack(int mask) {
		GLM::glStencilMaskSeparate(GL_BACK, mask);
	}

	void IGL_OtherSingle::stencilFuncFront(int func, int ref, int mask) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilFuncFront(func, ref, mask);
		});
	}
	void IGL_OtherSingle::stencilFuncBack(int func, int ref, int mask) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilFuncBack(func, ref, mask);
		});
	}
	void IGL_OtherSingle::stencilOpFront(int sfail, int dpfail, int dppass) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilOpFront(sfail, dpfail, dppass);
		});
	}
	void IGL_OtherSingle::stencilOpBack(int sfail, int dpfail, int dppass) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilOpBack(sfail, dpfail, dppass);
		});
	}
	void IGL_OtherSingle::stencilMaskFront(int mask) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilMaskFront(mask);
		});
	}
	void IGL_OtherSingle::stencilMaskBack(int mask) {
		GLM::s_drawHandler->postExec([=](){
			IGL_Draw().stencilMaskBack(mask);
		});
	}
	namespace {
		template <class RET>
		struct CallHandler {
			template <class CB>
			RET operator()(Handler& h, CB cb) const {
				RET ret;
				h.postExec([&](){
					ret = cb();
				});
				return ret;
			}
		};
		template <>
		struct CallHandler<void> {
			template <class CB>
			void operator()(Handler& h, CB cb) const {
				h.postExec([&](){
					cb();
				});
			}
		};
	}
	// マクロで分岐
 	#define DEF_GLMETHOD(ret_type, name, args, argnames) \
		typename GLM::t_##name GLM::name = nullptr; \
 		ret_type IGL_Draw::name(BOOST_PP_SEQ_ENUM(args)) { \
 			return GLM::name(BOOST_PP_SEQ_ENUM(argnames)); } \
 		ret_type IGL_OtherSingle::name(BOOST_PP_SEQ_ENUM(args)) { \
			return CallHandler<ret_type>()(*GLM::s_drawHandler, [=](){ \
				return IGL_Draw().name(BOOST_PP_SEQ_ENUM(argnames)); }); }

		#ifndef ANDROID
			#ifdef ANDROID
				#include "android_gl.inc"
			#elif defined(WIN32)
				#include "mingw_gl.inc"
			#else
				#include "linux_gl.inc"
			#endif
		#endif

	#undef DEF_GLMETHOD
	
	bool GLM::s_bShare = false;
	IGL_Draw GLM::s_ctxDraw;
	IGL_OtherSingle GLM::s_ctxSingle;
	Handler* GLM::s_drawHandler = nullptr;

	void GLM::SetShareMode(bool bShareEnabled) {
		s_bShare = bShareEnabled;
	}
	void GLM::InitializeMainThread() {
		Assert(Trap, !tls_GL.initialized())
		if(s_bShare)
			tls_GL = &s_ctxDraw;
		else
			tls_GL = &s_ctxSingle;
	}
	void GLM::InitializeDrawThread(Handler& handler) {
		Assert(Trap, !tls_GL.initialized())
		s_drawHandler = &handler;
		tls_GL = &s_ctxDraw;
	}
	void GLM::TerminateDrawThread() {
		Assert(Trap, tls_GL.initialized())
		Assert(Trap, s_drawHandler)
		tls_GL.terminate();
		s_drawHandler = nullptr;
	}
}
