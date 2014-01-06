#include "sdlwrap.hpp"

namespace rs {
	void Window::SetStdGLAttributes(int major, int minor, int depth) {
		SetGLAttributes(SDL_GL_CONTEXT_MAJOR_VERSION, major,
						SDL_GL_CONTEXT_MINOR_VERSION, minor,
						SDL_GL_DOUBLEBUFFER, 1,
						SDL_GL_DEPTH_SIZE, depth);
	}
	SPWindow Window::Create(const std::string& title, int w, int h, uint32_t flag) {
		return Create(title, 128, 128, w, h, flag|SDL_WINDOW_OPENGL);
	}
	SPWindow Window::Create(const std::string& title, int x, int y, int w, int h, uint32_t flag) {
		return SPWindow(new Window(SDLEC_P(Warn, SDL_CreateWindow, title.c_str(), x, y, w, h, flag|SDL_WINDOW_OPENGL)));
	}
	Window::Window(SDL_Window* w): _window(w) {
		_checkState();
	}
	Window::~Window() {
		SDLEC_P(Warn, SDL_DestroyWindow, _window);
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
		SDLEC_P(Warn, SDL_SetWindowFullscreen, _window, bFull ? SDL_WINDOW_FULLSCREEN : 0);
		_checkState();
	}
	void Window::setGrab(bool bGrab) {
		SDLEC_P(Warn, SDL_SetWindowGrab, _window, bGrab ? SDL_TRUE : SDL_FALSE);
	}
	void Window::setMaximumSize(int w, int h) {
		SDLEC_P(Warn, SDL_SetWindowMaximumSize, _window, w, h);
	}
	void Window::setMinimumSize(int w, int h) {
		SDLEC_P(Warn, SDL_SetWindowMinimumSize, _window, w, h);
	}
	void Window::setSize(int w, int h) {
		SDLEC_P(Warn, SDL_SetWindowSize, _window, w, h);
	}
	void Window::show(bool bShow) {
		if(bShow)
			SDLEC_P(Warn, SDL_ShowWindow, _window);
		else
			SDLEC_P(Warn, SDL_HideWindow, _window);
		_checkState();
	}
	void Window::setTitle(const std::string& title) {
		SDLEC_P(Warn, SDL_SetWindowTitle, _window, title.c_str());
	}
	void Window::maximize() {
		SDLEC_P(Warn, SDL_MaximizeWindow, _window);
		_checkState();
	}
	void Window::minimize() {
		SDLEC_P(Warn, SDL_MinimizeWindow, _window);
		_checkState();
	}
	void Window::restore() {
		SDLEC_P(Warn, SDL_RestoreWindow, _window);
		_checkState();
	}
	void Window::setPosition(int x, int y) {
		SDLEC_P(Warn, SDL_SetWindowPosition, _window, x, y);
	}
	void Window::raise() {
		SDLEC_P(Warn, SDL_RaiseWindow, _window);
		_checkState();
	}
	uint32_t Window::getID() const {
		return SDLEC_P(Warn, SDL_GetWindowID, _window);
	}
	Window::Stat Window::getState() const {
		return _stat;
	}
	bool Window::isGrabbed() const {
		return SDLEC_P(Warn, SDL_GetWindowGrab, _window) == SDL_TRUE;
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
	namespace {
		spn::Size GetSize(void (*func)(SDL_Window*,int*,int*), SDL_Window* wd) {
			int w,h;
			SDLEC_P(Warn, func, wd, &w, &h);
			return spn::Size(w,h);
		}
	}
	spn::Size Window::getSize() const {
		return GetSize(SDL_GetWindowSize, _window);
	}
	spn::Size Window::getMaximumSize() const {
		return GetSize(SDL_GetWindowMaximumSize, _window);
	}
	spn::Size Window::getMinimumSize() const {
		return GetSize(SDL_GetWindowMinimumSize, _window);
	}
	uint32_t Window::getSDLFlag() const {
		return SDLEC_P(Warn, SDL_GetWindowFlags, _window);
	}
	SDL_Window* Window::getWindow() const {
		return _window;
	}

	void Window::EnableScreenSaver(bool bEnable) {
		if(bEnable)
			SDLEC_P(Warn, SDL_EnableScreenSaver);
		else
			SDLEC_P(Warn, SDL_DisableScreenSaver);
	}
}
