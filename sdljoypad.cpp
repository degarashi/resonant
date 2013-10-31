#include "input_dep_sdl.hpp"
#include "common.hpp"

namespace rs {
	int SDLJoypad::NumJoypad() {
		return SDL_NumJoysticks();
	}
	void SDLJoypad::Update() {
		SDL_JoystickEventState(SDL_IGNORE);
		SDL_JoystickUpdate();
	}
	SDLJoypad::SDLJoypad(SDL_Joystick* jp): _joypad(jp), _name(SDL_JoystickName(jp)), _attachFlag(~0) {}
	int SDLJoypad::dep_numButtons() const {
		return SDL_JoystickNumButtons(_joypad);
	}
	int SDLJoypad::dep_numAxes() const {
		return SDL_JoystickNumAxes(_joypad);
	}
	int SDLJoypad::dep_numHats() const {
		return SDL_JoystickNumHats(_joypad);
	}
	bool SDLJoypad::dep_scan() {
		if(SDL_JoystickGetAttached(_joypad) == SDL_TRUE) {
			_attachFlag = ~0;
			return true;
		}
		_attachFlag = 0;
		return false;
	}
	int SDLJoypad::dep_getButton(int num) const {
		int val = SDL_JoystickGetButton(_joypad, num);
		if(val == 0)
			return 0;
		return _attachFlag & InputRange;
	}
	int SDLJoypad::dep_getAxis(int num) const {
		return (SDL_JoystickGetAxis(_joypad, num) >> 5) & _attachFlag;
	}
	int SDLJoypad::dep_getHat(int num) const {
		int unit = InputRange / 8;
		int res = -1;
		switch(SDL_JoystickGetHat(_joypad, num)) {
			case SDL_HAT_CENTERED: res = -1;
			case SDL_HAT_UP: res = 0;
			case SDL_HAT_RIGHTUP: res = unit;
			case SDL_HAT_RIGHT: res = unit*2;
			case SDL_HAT_RIGHTDOWN: res = unit*3;
			case SDL_HAT_DOWN: res = unit*4;
			case SDL_HAT_LEFTDOWN: res = unit*5;
			case SDL_HAT_LEFT: res = unit*6;
			case SDL_HAT_LEFTUP: res = unit*7;
		}
		return static_cast<int>(res | ~_attachFlag);
	}
	const std::string& SDLJoypad::name() const {
		return _name;
	}
	SDLJoypad::~SDLJoypad() {
		SDL_JoystickClose(_joypad);
	}
	void SDLJoypad::Initialize() {
		SDL_InitSubSystem(SDL_INIT_JOYSTICK);
	}
	void SDLJoypad::Terminate() {
		SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
	}
}
