#include "sdlwrap.hpp"

namespace rs {
	GLContext::GLContext(const SPWindow& w) {
		_ctx = SDLEC(Trap, SDL_GL_CreateContext, w->getWindow());
	}
	SPGLContext GLContext::CreateContext(const SPWindow& w, bool bShare) {
		Window::SetGLAttributes(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, bShare ? 1 : 0);
		return SPGLContext(new GLContext(w));
	}
	GLContext::~GLContext() {
		makeCurrent();
		SDLEC_P(Trap, SDL_GL_DeleteContext, _ctx);
	}
	void GLContext::makeCurrent(const SPWindow& w) {
		SDLEC_P(Trap, SDL_GL_MakeCurrent, w->getWindow(), _ctx);
		_spWindow = w;
	}
	void GLContext::makeCurrent() {
		SDLEC_P(Trap, SDL_GL_MakeCurrent, nullptr, nullptr);
		_spWindow = nullptr;
	}
	void GLContext::swapWindow() {
		if(_spWindow)
			SDLEC_P(Warn, SDL_GL_SwapWindow, _spWindow->getWindow());
	}
	int GLContext::SetSwapInterval(int n) {
		return SDL_GL_SetSwapInterval(n);
	}
}
