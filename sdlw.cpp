#include "sdlwrap.hpp"
#include <iostream>

namespace rs {
	// -------------------- SDLError --------------------
	std::string SDLError::s_errString;
	const char* SDLError::ErrorDesc() {
		const char* err = SDL_GetError();
		if(*err != '\0') {
			s_errString = err;
			SDL_ClearError();
			return s_errString.c_str();
		}
		return nullptr;
	}
	const char* SDLError::GetAPIName() {
		return "SDL";
	}
	SDL_threadID thread_local tls_threadID(~0);

	// -------------------- IMGError --------------------
	std::string IMGError::s_errString;
	const char* IMGError::ErrorDesc() {
		const char* err = IMG_GetError();
		if(*err != '\0') {
			s_errString = err;
			IMG_SetError("");
			return s_errString.c_str();
		}
		return nullptr;
	}
	const char* IMGError::GetAPIName() {
		return "SDL2_image";
	}
}
