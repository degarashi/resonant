#include "input_dep_sdl.hpp"

namespace rs {
	HLInput SDLTouchpad::s_hlInput;
	SDLTouchpad::EventQ SDLTouchpad::s_eventQ[2];
	int SDLTouchpad::s_evSw = 0;
	SDLTouchpad::SDLTouchpad(int touchId): _touchId(touchId) {}
	bool SDLTouchpad::dep_scan(IRecvPointer* r) {
		// 貯めておいたイベントを処理
		auto& q = s_eventQ[s_evSw][_touchId];
		if(!q.empty()) {
			// ポインタが増える際はrのaddNewPointer()を呼ぶ
			for(auto& e : q) {
				auto& f = e.tfinger;
				auto itr = _fmap.find(f.fingerId);
				switch(e.type) {
					case SDL_FINGERDOWN: {
						HLPtr hlP = mgr_pointer.acquire(TPos2D());
						r->newPointer(hlP.weak());
						_fmap[f.fingerId] = std::move(hlP);
					} break;
					case SDL_FINGERMOTION:
						if(itr != _fmap.end())
							itr->second.ref().setNewAbs(spn::Vec2(f.dx, f.dy));
						break;
					case SDL_FINGERUP:
						if(itr != _fmap.end())
							_fmap.erase(itr);
						break;
				}
			}
			q.clear();
		}
		return true;
	}
	int SDLTouchpad::NumTouchpad() {
		return 1;
	}
	namespace {
		std::string c_name("(default touchpad)");
	}
	std::string SDLTouchpad::GetTouchpadName(int /*num*/) {
		return c_name;
	}
	int SDLTouchpad::ProcessEvent(void*, SDL_Event* e) {
		// タッチパネル関係のイベントだけコピー
		if(e->type >= SDL_FINGERDOWN &&
			e->type <= SDL_FINGERMOTION)
		{
			s_eventQ[s_evSw^1][e->tfinger.touchId].push_back(*e);
			return 1;
		}
		return 0;
	}
	void SDLTouchpad::_Initialize() {
		SDL_AddEventWatch(SDLTouchpad::ProcessEvent, nullptr);
	}
	void SDLTouchpad::Update() {
		s_eventQ[s_evSw].clear();
		s_evSw ^= 1;
	}
	void SDLTouchpad::Terminate() {
		if(s_hlInput.valid()) {
			SDL_DelEventWatch(SDLTouchpad::ProcessEvent, nullptr);
			s_hlInput.release();
		}
	}
	const std::string& SDLTouchpad::name() const {
		return c_name;
	}
	WPtr SDLTouchpad::dep_getPointer() const {
		// 何かFingerIDがあれば適当に1つ返す
		if(!_fmap.empty())
			return ((_fmap.begin())->second).weak();
		return WPtr();
	}
}
