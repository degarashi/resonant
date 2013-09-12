#include "sdlwrap.hpp"

namespace sdlw {
	Mutex::Mutex(): _mutex(SDL_CreateMutex()) {}
	Mutex::~Mutex() { SDL_DestroyMutex(_mutex); }

	bool Mutex::lock() {
		int res = SDL_LockMutex(_mutex);
		SDLW_ACheck
		return res == 0;
	}
	bool Mutex::try_lock() {
		int res = SDL_TryLockMutex(_mutex);
		SDLW_ACheck
		return res == 0;
	}
	void Mutex::unlock() {
		SDL_UnlockMutex(_mutex);
		SDLW_ACheck
	}
	SDL_mutex* Mutex::getMutex() {
		return _mutex;
	}
}
