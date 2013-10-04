#include "sdlwrap.hpp"

namespace sdlw {
	Mutex::Mutex(): _mutex(SDL_CreateMutex()) {}
	Mutex::~Mutex() { SDL_DestroyMutex(_mutex); }

	bool Mutex::lock() {
		return SDLECA(SDL_LockMutex, _mutex) == 0;
	}
	bool Mutex::try_lock() {
		return SDLECA(SDL_TryLockMutex, _mutex) == 0;
	}
	void Mutex::unlock() {
		SDLECA(SDL_UnlockMutex, _mutex);
	}
	SDL_mutex* Mutex::getMutex() {
		return _mutex;
	}
}
