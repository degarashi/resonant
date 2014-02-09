#include "sdlwrap.hpp"
#include <iostream>

namespace rs {
	TLS<SDL_threadID> tls_threadID(~0);
	// -------------------- SDLInitializer --------------------
	SDLInitializer::SDLInitializer(uint32_t flag) {
		SDL_Init(flag);
	}
	SDLInitializer::~SDLInitializer() {
		SDL_Quit();
	}
	// -------------------- IMGInitializer --------------------
	IMGInitializer::IMGInitializer(uint32_t flag) {
		IMG_Init(flag);
	}
	IMGInitializer::~IMGInitializer() {
		IMG_Quit();
	}
	// -------------------- SDLError --------------------
	const char* SDLErrorI::Get() { return SDL_GetError(); }
	void SDLErrorI::Reset() { SDL_ClearError(); }
	const char *const SDLErrorI::c_apiName = "SDL2";
	// -------------------- IMGError --------------------
	const char* IMGErrorI::Get() { return IMG_GetError(); }
	void IMGErrorI::Reset() { IMG_SetError(""); }
	const char *const IMGErrorI::c_apiName = "SDL2_image";
}
