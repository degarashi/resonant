#include "input_dep_sdl.hpp"
#include "common.hpp"

namespace rs {
	HLInput SDLKeyboard::s_hlInput;
	int SDLKeyboard::dep_getButton(int num) const {
		return (_state[num]) ? InputRange : 0;
	}
	bool SDLKeyboard::dep_scan() {
		std::memcpy(&_state[0], SDL_GetKeyboardState(nullptr), SDL_NUM_SCANCODES);
		return true;
	}
	void SDLKeyboard::Update() {}
	void SDLKeyboard::Terminate() {
		s_hlInput.release();
	}
	const std::string& SDLKeyboard::name() const {
		static std::string str("(default keyboard)");
		return str;
	}
}
