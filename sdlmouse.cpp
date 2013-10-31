#include "input_dep_sdl.hpp"
#include "common.hpp"
#include "spinner/misc.hpp"

namespace rs {
	HLInput SDLMouse::s_hlInput;
	SDL_Window* SDLMouse::s_window(nullptr);
	SDLMouse::SDLMouse(): _state(0), _mode(MouseMode::Absolute) {}
	int SDLMouse::NumMouse() {
		return 1;
	}
	std::string SDLMouse::GetMouseName(int num) {
		return "(default mouse)";
	}
	int SDLMouse::dep_numButtons() const {
		return 3;
	}
	int SDLMouse::dep_numAxes() const {
		return 2;
	}
	int SDLMouse::dep_getButton(int num) const {
		return (_state & SDL_BUTTON(num+1)) ? InputRange : 0;
	}
	bool SDLMouse::dep_scan(TPos2D& t) {
		int x, y;
		_state = SDL_GetMouseState(&x, &y);
		// ウィンドウからカーソルが出ない様にする
		if(s_window) {
			if(_mode == MouseMode::Relative) {
				// カーソルを常にウィンドウ中央へセット
				int wx, wy;
				SDL_GetWindowSize(s_window, &wx, &wy);
				SDL_WarpMouseInWindow(s_window, wx/2, wy/2);
			}
		}
		t.setNewAbs(spn::Vec2(x,y));
		// マウスは常に接続されている前提
		return true;
	}
	void SDLMouse::Terminate() {
		s_hlInput.release();
	}
	void SDLMouse::dep_setMode(MouseMode mode, TPos2D& t) {
		_mode = mode;
		SDL_bool b = mode!=MouseMode::Absolute ? SDL_TRUE : SDL_FALSE;
		SDL_SetWindowGrab(s_window, b);
		SDL_SetRelativeMouseMode(b);
		if(mode == MouseMode::Relative) {
			int wx, wy;
			SDL_GetWindowSize(s_window, &wx, &wy);
			SDL_WarpMouseInWindow(s_window, wx/2, wy/2);
			// カーソルをウィンドウ中央へセットした後に相対移動距離をリセット
			t.absPos = spn::Vec2(wx/2, wy/2);
			t.relPos = spn::Vec2(0);
		}
	}
	MouseMode SDLMouse::dep_getMode() const {
		return _mode;
	}
	void SDLMouse::SetWindow(SDL_Window* w) {
		s_window = w;
	}
}
