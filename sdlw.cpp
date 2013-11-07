#include "sdlwrap.hpp"
#include <iostream>

namespace rs {
	SDL_threadID thread_local tls_threadID(~0);
	// -------------------- SDLInitializer --------------------
	SDLInitializer::SDLInitializer(uint32_t flag) {
		SDL_Init(flag);
	}
	SDLInitializer::~SDLInitializer() {
		SDL_Quit();
	}
	// -------------------- SDLError --------------------
	const char* SDLErrorI::Get() { return SDL_GetError(); }
	void SDLErrorI::Reset() { SDL_ClearError(); }
	const char *const SDLErrorI::c_apiName = "SDL2";
	// -------------------- IMGError --------------------
	const char* IMGErrorI::Get() { return IMG_GetError(); }
	void IMGErrorI::Reset() { IMG_SetError(""); }
	const char *const IMGErrorI::c_apiName = "SDL2_image";
	// -------------------- TTFError --------------------
	const char* TTFErrorI::Get() { return TTF_GetError(); }
	void TTFErrorI::Reset() { TTF_SetError(""); }
	const char *const TTFErrorI::c_apiName = "SDL2_ttf";
}
