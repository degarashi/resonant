#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"

namespace {
	constexpr int RESOLUTION_X = 1024,
				RESOLUTION_Y = 768;
	constexpr char TITLE[] = "RSE-Test v0.01",
				APP_NAME[] = "rse_test",
				ORG_NAME[] = "degarashi";
}

int main(int argc, char **argv) {
	// 第一引数にpathlistファイルの指定が必須
	if(argc <= 1) {
		LogOutput("usage: rse_demo pathlist_file");
		return 0;
	}
	rs::GameLoop gloop([](const rs::SPWindow& sp){ return new MyMain(sp); },
					[](){ return new MyDraw; });

	rs::GLoopInitParam param;
	auto& wp = param.wparam;
	wp.title = TITLE;
	wp.width = RESOLUTION_X;
	wp.height = RESOLUTION_Y;
	wp.flag = SDL_WINDOW_SHOWN;
	auto& gp = param.gparam;
	gp.depth = 24;
	gp.verMajor = 2;
	gp.verMinor = 0;
	param.pathfile = argv[1];
	param.app_name = APP_NAME;
	param.organization = ORG_NAME;
	return gloop.run(param);
}
