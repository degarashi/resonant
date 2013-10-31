#include "sdlwrap.hpp"

namespace rs {
	GLContext::GLContext(const SPWindow& w) {
		_ctx = SDL_GL_CreateContext(w->getWindow());
	}
	SPGLContext GLContext::CreateContext(const SPWindow& w) {
		return SPGLContext(new GLContext(w));
	}
	GLContext::~GLContext() {
		SDL_GL_DeleteContext(_ctx);
	}
	void GLContext::makeCurrent(const SPWindow& w) {
		SDL_GL_MakeCurrent(w->getWindow(), _ctx);
		_spWindow = w;
	}
	void GLContext::makeCurrent() {
		SDL_GL_MakeCurrent(nullptr, nullptr);
		_spWindow = nullptr;
	}
	void GLContext::swapWindow() {
		if(_spWindow)
			SDL_GL_SwapWindow(_spWindow->getWindow());
	}
	int GLContext::SetSwapInterval(int n) {
		return SDL_GL_SetSwapInterval(n);
	}
}
