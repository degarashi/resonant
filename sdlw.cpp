#include "sdlwrap.hpp"
#include <iostream>

namespace sdlw {
	SDL_threadID thread_local tls_threadID(0);
	void CheckSDLError(int line) {
		const char* error = SDL_GetError();
		if(*error != '\0') {
			std::cout << "SDL Error: " << error << std::endl;
			if(line != -1)
				std::cout << "on line: " << line << std::endl;
			SDL_ClearError();
		}
	}
}
