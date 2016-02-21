#include "gameloop.hpp"

namespace rs {
	const PrintEvent::TypeP PrintEvent::cs_type[NUM_EVENT] = {
		{[](uint32_t id) { return id == SDL_WINDOWEVENT; }, Window},
		{[](uint32_t id) { return id>=SDL_FINGERDOWN && id<= SDL_FINGERMOTION; }, Touch}
	};

	bool PrintEvent::Window(const SDL_Event& e) {
		switch(e.window.event) {
			case SDL_WINDOWEVENT_HIDDEN:
				::spn::Log::Output("Window %d hidden", e.window.windowID); break;
			case SDL_WINDOWEVENT_EXPOSED:
				::spn::Log::Output("Window %d exposed", e.window.windowID); break;
			case SDL_WINDOWEVENT_MOVED:
				::spn::Log::Output("Window %d moved to %d,%d",
                    e.window.windowID, e.window.data1,
                    e.window.data2); break;
			case SDL_WINDOWEVENT_RESIZED:
				::spn::Log::Output("Window %d resized to %dx%d",
                    e.window.windowID, e.window.data1,
                    e.window.data2); break;
			case SDL_WINDOWEVENT_MINIMIZED:
				::spn::Log::Output("Window %d minimized", e.window.windowID); break;
			case SDL_WINDOWEVENT_MAXIMIZED:
				::spn::Log::Output("Window %d maximized", e.window.windowID); break;
			case SDL_WINDOWEVENT_RESTORED:
				::spn::Log::Output("Window %d restored", e.window.windowID); break;
			case SDL_WINDOWEVENT_ENTER:
				::spn::Log::Output("Mouse entered window %d", e.window.windowID); break;
			case SDL_WINDOWEVENT_LEAVE:
				::spn::Log::Output("Mouse left window %d", e.window.windowID); break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				::spn::Log::Output("Window %d gained keyboard focus",
                    e.window.windowID); break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				::spn::Log::Output("Window %d lost keyboard focus",
                    e.window.windowID); break;
			case SDL_WINDOWEVENT_CLOSE:
				::spn::Log::Output("Window %d shown", e.window.windowID); break;
			default:
				::spn::Log::Output("Window %d got unknown event %d",
                    e.window.windowID, e.window.event);
				return false;
		}
		return true;
	}
	bool PrintEvent::Touch(const SDL_Event& e) {
		auto printCoord = [](const SDL_Event& e, const char* act) {
			auto& tf = e.tfinger;
			::spn::Log::Output("Touch %d  finger %s %d", tf.touchId, act, tf.fingerId);
			::spn::Log::Output("x=%f y=%f dx=%f dy=%f pressure=%f", tf.x, tf.y, tf.dx, tf.dy, tf.pressure);
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
