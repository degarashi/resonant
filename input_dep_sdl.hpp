#pragma once
#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_mouse.h>
#include <SDL2/SDL_keyboard.h>
#include <SDL2/SDL_joystick.h>
#include <string>
#include <bitset>
#include <vector>
#include <map>
#include "common.hpp"

namespace rs {
	class SDLKeyboard {
		// 単純にSDLのフラグをコピー
		std::array<uint8_t, SDL_NUM_SCANCODES>	_state;
		static HLInput s_hlInput;
		protected:
			int dep_getButton(int num) const;
			bool dep_scan();
			// キーボードは1つだけ想定
			template <class T>
			static HLInput OpenKeyboard() {
				if(!s_hlInput.valid())
					s_hlInput = mgr_inputb.acquire(std::unique_ptr<T>(new T()));
				return s_hlInput;
			}
		public:
			static void Update();
			static void Terminate();
	};
	using KeyboardDep = SDLKeyboard;

	class SDLMouse {
		uint32_t 	_state;
		MouseMode	_mode;
		// SDLのマウスは1つだけを想定
		// OpenMouseは共通のハンドルを返す
		static HLInput		s_hlInput;
		static SDL_Window*	s_window;
		protected:
			template <class T>
			static HLInput OpenMouse(int num) {
				if(!s_hlInput.valid())
					s_hlInput = mgr_inputb.acquire(std::unique_ptr<T>(new T()));
				return s_hlInput;
			}
			int dep_numButtons() const;
			int dep_numAxes() const;
			int dep_getButton(int num) const;
			bool dep_scan(TPos2D& t);
			void dep_setMode(MouseMode mode, TPos2D& t);
			MouseMode dep_getMode() const;
			SDLMouse();
		public:
			static void Terminate();
			static int NumMouse();
			static void SetWindow(SDL_Window* w);
			static std::string GetMouseName(int num);
			const std::string& name() const;
	};
	using MouseDep = SDLMouse;

	// SDLにてJoypadを列挙し、指定のパッドのインタフェースを生成
	class SDLJoypad {
		SDL_Joystick* 	_joypad;
		std::string		_name;
		uint32_t		_attachFlag;

		protected:
			int dep_numButtons() const;
			int dep_numAxes() const;
			int dep_numHats() const;
			int dep_getButton(int num) const;
			int dep_getAxis(int num) const;
			int dep_getHat(int num) const;
			bool dep_scan();
			template <class T>
			static HLInput OpenJoypad(int num) {
				return mgr_inputb.acquire(std::unique_ptr<T>(new T(SDL_JoystickOpen(num))));
			}
		public:
			SDLJoypad(SDL_Joystick* jp);
			~SDLJoypad();

			static void Initialize();
			static void Update();
			static void Terminate();
			static int NumJoypad();
			static std::string GetJoypadName(int num);
			const std::string& name() const;
	};
	using JoypadDep = SDLJoypad;

	class SDLTouchpad {
		// TouchpadID -> Queue
		using EventQ = std::map<int, std::vector<SDL_Event>>;
		static EventQ	s_eventQ[2];
		static int		s_evSw;

		// FingerID -> PtrHandle
		using FMap = std::map<uint32_t, HLPtr>;
		FMap	_fmap;
		int		_touchId;
		static HLInput s_hlInput;
		protected:
			template <class T>
			static HLInput OpenTouchpad(int num) {
				if(!s_hlInput.valid()) {
					s_hlInput = mgr_inputb.acquire(std::unique_ptr<T>(new T(num)));
					_Initialize();
				}
				return s_hlInput;
			}
			bool dep_scan(IRecvPointer* r);
			static void _Initialize();
			WPtr dep_getPointer() const;
			const std::string& name() const;
		public:
			SDLTouchpad(int touchId);
			static void Update();
			static void Terminate();
			static int NumTouchpad();
			static std::string GetTouchpadName(int num);
			static int ProcessEvent(void*, SDL_Event* e);
	};
	using TouchDep = SDLTouchpad;
}
