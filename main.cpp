#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test/test.hpp"

constexpr int RESOLUTION_X = 1024,
			RESOLUTION_Y = 768;

int main(int argc, char **argv) {
	rs::GameLoop gloop([](const rs::SPWindow& sp){ return new MyMain(sp); },
					[](){ return new MyDraw; });
	return gloop.run(argv[0], "HelloSDL2", RESOLUTION_X, RESOLUTION_Y, SDL_WINDOW_SHOWN, 2,0,24);
}
