#include "gameloop.hpp"
#include "glhead.hpp"
#include "input.hpp"

class MyDraw : public rs::IDrawProc {
	public:
		bool runU(uint64_t accum) override {
			glClearColor(0,0,(accum&0xff) / 255.0f,1);
			glClear(GL_COLOR_BUFFER_BIT);
			return true;
		}
};
class MyMain : public rs::IMainProc {
	rs::HLInput hlIk;
	rs::HLAct actQuit,
			actButton;
	public:
		MyMain() {
			hlIk = rs::Keyboard::OpenKeyboard();
			actQuit = mgr_input.addAction("quit");
			mgr_input.link(actQuit.get(), rs::InF::AsButton(hlIk.get(), SDL_SCANCODE_ESCAPE));
			actButton = mgr_input.addAction("button");
			mgr_input.link(actQuit.get(), rs::InF::AsButton(hlIk.get(), SDL_SCANCODE_A));
		}
		bool runU() override {
			if(mgr_input.isKeyPressed(actQuit.get()))
				return false;
			return true;
		}
		rs::IDrawProc* initDraw() override {
			return new MyDraw;
		}
};

using namespace rs;
int main(int argc, char **argv) {
	return GameLoop([](){ return new MyMain; }, "HelloSDL2", 640, 480, SDL_WINDOW_SHOWN, 4,2,24);
}
