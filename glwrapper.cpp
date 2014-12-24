#include "glhead.hpp"
#include "event.hpp"

namespace rs {
	TLS<IGL*>	tls_GL;
	void IGL_Draw::stencilFuncFront(int func, int ref, int mask) {
		GLWrap::glStencilFuncSeparate(GL_FRONT, func, ref, mask);
	}
	void IGL_Draw::stencilFuncBack(int func, int ref, int mask) {
		GLWrap::glStencilFuncSeparate(GL_BACK, func, ref, mask);
	}
	void IGL_Draw::stencilOpFront(int sfail, int dpfail, int dppass) {
		GLWrap::glStencilOpSeparate(GL_FRONT, sfail, dpfail, dppass);
	}
	void IGL_Draw::stencilOpBack(int sfail, int dpfail, int dppass) {
		GLWrap::glStencilOpSeparate(GL_BACK, sfail, dpfail, dppass);
	}
	void IGL_Draw::stencilMaskFront(int mask) {
		GLWrap::glStencilMaskSeparate(GL_FRONT, mask);
	}
	void IGL_Draw::stencilMaskBack(int mask) {
		GLWrap::glStencilMaskSeparate(GL_BACK, mask);
	}

	void IGL_OtherSingle::stencilFuncFront(int func, int ref, int mask) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().stencilFuncFront(func, ref, mask);
		});
	}
	void IGL_OtherSingle::stencilFuncBack(int func, int ref, int mask) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().stencilFuncBack(func, ref, mask);
		});
	}
	void IGL_OtherSingle::stencilOpFront(int sfail, int dpfail, int dppass) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().stencilOpFront(sfail, dpfail, dppass);
		});
	}
	void IGL_OtherSingle::stencilOpBack(int sfail, int dpfail, int dppass) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().stencilOpBack(sfail, dpfail, dppass);
		});
	}
	void IGL_OtherSingle::stencilMaskFront(int mask) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
			IGL_Draw().stencilMaskFront(mask);
		});
	}
	void IGL_OtherSingle::stencilMaskBack(int mask) {
		auto p = GLW.refShared().put();
		GLW.getDrawHandler().postExec([=](){
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
	#define GLCall(func, seq)	GLWrap::func(BOOST_PP_SEQ_ENUM(seq));
	#define DEF_SINGLE_METHOD(ret_type, name, args, argnames) \
		ret_type IGL_OtherSingle::name(BOOST_PP_SEQ_ENUM(args)) { \
			auto p = GLW.refShared().put(); \
			return CallHandler<ret_type>()(GLW.getDrawHandler(), [=](){ \
				return IGL_Draw().name(BOOST_PP_SEQ_ENUM(argnames)); }); }
#ifdef DEBUG
	/* デバッグ時:	glFUNC = チェック付き
					glFUNC_NC = チェック無し */
	#define DEF_DRAW_GLEC(act, func, seq) \
		if((uintptr_t)GLWrap::func != (uintptr_t)GLWrap::glGetError) \
			return GLEC_Base(AAct_##act<::rs::GLE_Error>(), [&](){return GLWrap::func(BOOST_PP_SEQ_ENUM(seq));}); \
		return GLCall(func, seq)
#else
	#define DEF_DRAW_GLEC(act, func, seq) \
		return GLCall(func, seq)
#endif

	// マクロで分岐
	#define GLDEFINE(...)
	#define DEF_GLMETHOD(ret_type, num, name, args, argnames) \
		typename GLWrap::t_##name GLWrap::name = nullptr; \
		ret_type IGL_Draw::name##_NC(BOOST_PP_SEQ_ENUM(args)) { \
			return GLCall(name, argnames) } \
 		ret_type IGL_Draw::name(BOOST_PP_SEQ_ENUM(args)) { \
			DEF_DRAW_GLEC(Warn, name, argnames) } \
		DEF_SINGLE_METHOD(ret_type, name, args, argnames) \
		DEF_SINGLE_METHOD(ret_type, BOOST_PP_CAT(name, _NC), args, argnames)

		#ifdef ANDROID
			#include "android_gl.inc"
		#elif defined(WIN32)
			#include "mingw_gl.inc"
		#else
			#include "linux_gl.inc"
		#endif

	#undef DEF_SINGLE_METHOD
	#undef DEF_GLMETHOD
	#undef GLDEFINE
	#undef DEF_DRAW_METHOD
	#undef DEF_DRAW_GLEC
	#undef GLCall

	GLWrap::GLWrap(bool bShareEnabled): _bShare(bShareEnabled), _drawHandler(nullptr) {
		*_pShared.lock() = nullptr;
	}

	void GLWrap::initializeMainThread() {
		Assert(Trap, !tls_GL.initialized())
		if(_bShare)
			tls_GL = &_ctxDraw;
		else
			tls_GL = &_ctxSingle;
	}
	void GLWrap::initializeDrawThread(Handler& handler) {
		Assert(Trap, !tls_GL.initialized())
		_drawHandler = &handler;
		tls_GL = &_ctxDraw;
	}
	void GLWrap::terminateDrawThread() {
		Assert(Trap, tls_GL.initialized())
		Assert(Trap, _drawHandler)
		tls_GL.terminate();
		_drawHandler = nullptr;
	}
	Handler& GLWrap::getDrawHandler() {
		AssertP(Trap, _drawHandler)
		return *_drawHandler;
	}
	GLWrap::Shared& GLWrap::refShared() {
		return _pShared;
	}
}
