#include "sdlwrap.hpp"

namespace rs {
	// --------------------- Param ---------------------
	Window::Param::Param() {
		posx = posy = SDL_WINDOWPOS_UNDEFINED;
		width = 640;
		height = 480;
		flag = SDL_WINDOW_SHOWN;
	}
	Window::GLParam::GLParam() {
		verMajor = 2;
		verMinor = 0;
		red = green = blue = 5;
		depth = 16;
		doublebuffer = 1;
	}
	namespace {
		const SDL_GLattr c_AttrId[] = {
			SDL_GL_CONTEXT_MAJOR_VERSION,
			SDL_GL_CONTEXT_MINOR_VERSION,
			SDL_GL_DOUBLEBUFFER,
			SDL_GL_RED_SIZE,
			SDL_GL_GREEN_SIZE,
			SDL_GL_BLUE_SIZE,
			SDL_GL_DEPTH_SIZE
		};
		using IPtr = int (Window::GLParam::*);
		const IPtr c_AttrPtr[] = {
			&Window::GLParam::verMajor,
			&Window::GLParam::verMinor,
			&Window::GLParam::doublebuffer,
			&Window::GLParam::red,
			&Window::GLParam::green,
			&Window::GLParam::blue,
			&Window::GLParam::depth
		};
	}
	void Window::GLParam::getStdAttributes() {
		for(int i=0 ; i<countof(c_AttrId) ; i++)
			Window::GetGLAttributes(c_AttrId[i], this->*c_AttrPtr[i]);
	}
	void Window::GLParam::setStdAttributes() const {
		for(int i=0 ; i<countof(c_AttrId) ; i++)
			Window::SetGLAttributes(c_AttrId[i], this->*c_AttrPtr[i]);

		SetGLAttributes(SDL_GL_CONTEXT_PROFILE_MASK,
						#ifdef USE_OPENGLES2
							SDL_GL_CONTEXT_PROFILE_ES,
						#else
							SDL_GL_CONTEXT_PROFILE_CORE,
						#endif
						SDL_GL_ACCELERATED_VISUAL, 1);
	}
	// --------------------- Window ---------------------
	SPWindow Window::Create(const Param& p) {
		return Create(p.title, p.posx, p.posy, p.width, p.height, p.flag);
	}
	SPWindow Window::Create(const std::string& title, int w, int h, uint32_t flag) {
		return Create(title, 128, 128, w, h, flag);
	}
	SPWindow Window::Create(const std::string& title, int x, int y, int w, int h, uint32_t flag) {
		return SPWindow(new Window(SDLEC_D(Warn, SDL_CreateWindow, title.c_str(), x, y, w, h, flag|SDL_WINDOW_OPENGL)));
	}
	Window::Window(SDL_Window* w): _window(w) {
		_checkState();
	}
	Window::~Window() {
		SDLEC_D(Warn, SDL_DestroyWindow, _window);
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
		SDLEC_D(Warn, SDL_SetWindowFullscreen, _window, bFull ? SDL_WINDOW_FULLSCREEN : 0);
		_checkState();
	}
	void Window::setGrab(bool bGrab) {
		SDLEC_D(Warn, SDL_SetWindowGrab, _window, bGrab ? SDL_TRUE : SDL_FALSE);
	}
	void Window::setMaximumSize(int w, int h) {
		SDLEC_D(Warn, SDL_SetWindowMaximumSize, _window, w, h);
	}
	void Window::setMinimumSize(int w, int h) {
		SDLEC_D(Warn, SDL_SetWindowMinimumSize, _window, w, h);
	}
	void Window::setSize(int w, int h) {
		SDLEC_D(Warn, SDL_SetWindowSize, _window, w, h);
	}
	void Window::show(bool bShow) {
		if(bShow)
			SDLEC_D(Warn, SDL_ShowWindow, _window);
		else
			SDLEC_D(Warn, SDL_HideWindow, _window);
		_checkState();
	}
	void Window::setTitle(const std::string& title) {
		SDLEC_D(Warn, SDL_SetWindowTitle, _window, title.c_str());
	}
	void Window::maximize() {
		SDLEC_D(Warn, SDL_MaximizeWindow, _window);
		_checkState();
	}
	void Window::minimize() {
		SDLEC_D(Warn, SDL_MinimizeWindow, _window);
		_checkState();
	}
	void Window::restore() {
		SDLEC_D(Warn, SDL_RestoreWindow, _window);
		_checkState();
	}
	void Window::setPosition(int x, int y) {
		SDLEC_D(Warn, SDL_SetWindowPosition, _window, x, y);
	}
	void Window::raise() {
		SDLEC_D(Warn, SDL_RaiseWindow, _window);
		_checkState();
	}
	uint32_t Window::getID() const {
		return SDLEC_D(Warn, SDL_GetWindowID, _window);
	}
	Window::Stat Window::getState() const {
		return _stat;
	}
	bool Window::isGrabbed() const {
		return SDLEC_D(Warn, SDL_GetWindowGrab, _window) == SDL_TRUE;
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
			SDLEC_D(Warn, func, wd, &w, &h);
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
		return SDLEC_D(Warn, SDL_GetWindowFlags, _window);
	}
	SDL_Window* Window::getWindow() const {
		return _window;
	}

	void Window::EnableScreenSaver(bool bEnable) {
		if(bEnable)
			SDLEC_D(Warn, SDL_EnableScreenSaver);
		else
			SDLEC_D(Warn, SDL_DisableScreenSaver);
	}
}
