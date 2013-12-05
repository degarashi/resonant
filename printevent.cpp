#include "gameloop.hpp"

namespace rs {
	const PrintEvent::TypeP PrintEvent::cs_type[NUM_EVENT] = {
		{[](uint32_t id) { return id == SDL_WINDOWEVENT; }, Window},
		{[](uint32_t id) { return id>=SDL_FINGERDOWN && id<= SDL_FINGERMOTION; }, Touch}
	};

	bool PrintEvent::Window(const SDL_Event& e) {
		switch(e.window.event) {
			case SDL_WINDOWEVENT_HIDDEN:
				LogOutput("Window %d hidden", e.window.windowID); break;
			case SDL_WINDOWEVENT_EXPOSED:
				LogOutput("Window %d exposed", e.window.windowID); break;
			case SDL_WINDOWEVENT_MOVED:
				LogOutput("Window %d moved to %d,%d",
                    e.window.windowID, e.window.data1,
                    e.window.data2); break;
			case SDL_WINDOWEVENT_RESIZED:
				LogOutput("Window %d resized to %dx%d",
                    e.window.windowID, e.window.data1,
                    e.window.data2); break;
			case SDL_WINDOWEVENT_MINIMIZED:
				LogOutput("Window %d minimized", e.window.windowID); break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				LogOutput("Window %d maximized", e.window.windowID); break;
			case SDL_WINDOWEVENT_RESTORED:
				LogOutput("Window %d restored", e.window.windowID); break;
			case SDL_WINDOWEVENT_ENTER:
				LogOutput("Mouse entered window %d", e.window.windowID); break;
			case SDL_WINDOWEVENT_LEAVE:
				LogOutput("Mouse left window %d", e.window.windowID); break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				LogOutput("Window %d gained keyboard focus",
                    e.window.windowID); break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				LogOutput("Window %d lost keyboard focus",
                    e.window.windowID); break;
			case SDL_WINDOWEVENT_CLOSE:
				LogOutput("Window %d shown", e.window.windowID); break;
			default:
				LogOutput("Window %d got unknown event %d",
                    e.window.windowID, e.window.event);
				return false;
		}
		return true;
	}
	bool PrintEvent::Touch(const SDL_Event& e) {
		auto printCoord = [](const SDL_Event& e, const char* act) {
			auto& tf = e.tfinger;
			LogOutput("Touch %d  finger %s %d", tf.touchId, act, tf.fingerId);
			LogOutput("x=%f y=%f dx=%f dy=%f pressure=%f", tf.x, tf.y, tf.dx, tf.dy, tf.pressure);
		};
		switch(e.type) {
			case SDL_FINGERDOWN:
				printCoord(e, "down"); break;
			case SDL_FINGERUP:
				printCoord(e, "up"); break;
			case SDL_FINGERMOTION:
				printCoord(e, "move"); break;
			default:
				return false;
		}
		return true;
	}
	void PrintEvent::All(const SDL_Event& e, uint32_t filter) {
		for(auto& t : cs_type) {
			if(filter == 0)
				break;
			if(t.checker(e.type))
				t.proc(e);
			filter >>= 1;
		}
	}
}
