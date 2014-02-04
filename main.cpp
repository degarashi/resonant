#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test/test.hpp"

constexpr int RESOLUTION_X = 1024,
			RESOLUTION_Y = 768;
constexpr char TITLE[] = "RSE-Test v0.01";

int main(int argc, char **argv) {
	// 第一引数にpathlistファイルの指定が必須
	if(argc <= 1) {
		LogOutput("usage: rse_demo pathlist_file");
		return 0;
	}
	rs::GameLoop gloop([](const rs::SPWindow& sp){ return new MyMain(sp); },
					[](){ return new MyDraw; });
	return gloop.run(argv[1], TITLE, RESOLUTION_X, RESOLUTION_Y, SDL_WINDOW_SHOWN, 2,0,24);
}
