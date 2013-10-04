#include "sdlwrap.hpp"
#include <iostream>

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

namespace sdlw {
	SDL_threadID thread_local tls_threadID(~0);
}
