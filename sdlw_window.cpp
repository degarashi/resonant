#include "sdlwrap.hpp"

namespace rs {
	void Window::SetStdGLAttributes(int major, int minor, int depth) {
		SetGLAttributes(SDL_GL_CONTEXT_MAJOR_VERSION, major,
						SDL_GL_CONTEXT_MINOR_VERSION, minor,
						SDL_GL_DOUBLEBUFFER, 1,
						SDL_GL_DEPTH_SIZE, depth);
	}
	SPWindow Window::Create(const std::string& title, int w, int h, uint32_t flag, bool bShare) {
		return Create(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, w, h, flag, bShare);
	}
	SPWindow Window::Create(const std::string& title, int x, int y, int w, int h, uint32_t flag, bool bShare) {
		SetGLAttributes(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, bShare ? 1 : 0);
		return SPWindow(new Window(SDL_CreateWindow(title.c_str(), x, y, w, h, flag|SDL_WINDOW_OPENGL)));
	}
	Window::Window(SDL_Window* w): _window(w) {
		_checkState();
	}
	Window::~Window() {
		SDL_DestroyWindow(_window);
	}
	void Window::_checkState() {
		uint32_t f = getSDLFlag();
		if(f & SDL_WINDOW_SHOWN)
			_stat = Stat::Hidden;
		else if(f & SDL_WINDOW_FULLSCREEN)
			_stat = Stat::Fullscreen;
		else if(f & SDL_WINDOW_MINIMIZED)
			_stat = Stat::Minimized;
		else if(f & SDL_WINDOW_MAXIMIZED)
			_stat = Stat::Maximized;
		else
			_stat = Stat::Shown;
	}
	void Window::setFullscreen(bool bFull) {
		SDL_SetWindowFullscreen(_window, bFull ? SDL_WINDOW_FULLSCREEN : 0);
		_checkState();
	}
	void Window::setGrab(bool bGrab) {
		SDL_SetWindowGrab(_window, bGrab ? SDL_TRUE : SDL_FALSE);
	}
	void Window::setMaximumSize(int w, int h) {
		SDL_SetWindowMaximumSize(_window, w, h);
	}
	void Window::setMinimumSize(int w, int h) {
		SDL_SetWindowMinimumSize(_window, w, h);
	}
	void Window::setSize(int w, int h) {
		SDL_SetWindowSize(_window, w, h);
	}
	void Window::show(bool bShow) {
		if(bShow)
			SDL_ShowWindow(_window);
		else
			SDL_HideWindow(_window);
		_checkState();
	}
	void Window::setTitle(const std::string& title) {
		SDL_SetWindowTitle(_window, title.c_str());
	}
	void Window::maximize() {
		SDL_MaximizeWindow(_window);
		_checkState();
	}
	void Window::minimize() {
		SDL_MinimizeWindow(_window);
		_checkState();
	}
	void Window::restore() {
		SDL_RestoreWindow(_window);
		_checkState();
	}
	void Window::setPosition(int x, int y) {
		SDL_SetWindowPosition(_window, x, y);
	}
	void Window::raise() {
		SDL_RaiseWindow(_window);
		_checkState();
	}
	uint32_t Window::getID() const {
		return SDL_GetWindowID(_window);
	}
	Window::Stat Window::getState() const {
		return _stat;
	}
	bool Window::isGrabbed() const {
		return SDL_GetWindowGrab(_window) == SDL_TRUE;
	}
	bool Window::isResizable() const {
		return getSDLFlag() & SDL_WINDOW_RESIZABLE;
	}
	bool Window::hasInputFocus() const {
		return getSDLFlag() & SDL_WINDOW_INPUT_FOCUS;
	}
	bool Window::hasMouseFocus() const {
		return getSDLFlag() & SDL_WINDOW_MOUSE_FOCUS;
	}
	uint32_t Window::getSDLFlag() const {
		return SDL_GetWindowFlags(_window);
	}
	SDL_Window* Window::getWindow() const {
		return _window;
	}

	void Window::EnableScreenSaver(bool bEnable) {
		if(bEnable)
			SDL_EnableScreenSaver();
		else
			SDL_DisableScreenSaver();
	}
}
