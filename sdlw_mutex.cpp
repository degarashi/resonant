#include "sdlwrap.hpp"

namespace rs {
	Mutex::Mutex(): _mutex(SDL_CreateMutex()) {}
	Mutex::Mutex(Mutex&& m): _mutex(m._mutex) {
		m._mutex = nullptr;
	}
	Mutex::~Mutex() {
		if(_mutex)
			SDL_DestroyMutex(_mutex);
	}
	bool Mutex::lock() {
		return SDLEC_P(Trap, SDL_LockMutex, _mutex) == 0;
	}
	bool Mutex::try_lock() {
		return SDLEC_P(Trap, SDL_TryLockMutex, _mutex) == 0;
	}
	void Mutex::unlock() {
		SDLEC_P(Trap, SDL_UnlockMutex, _mutex);
	}
	SDL_mutex* Mutex::getMutex() {
		return _mutex;
	}
}
