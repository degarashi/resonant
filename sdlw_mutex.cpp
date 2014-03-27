#include "sdlwrap.hpp"

namespace rs {
	// ----- Mutex -----
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

	// ----- UniLock -----
	UniLock::UniLock(Mutex& m):
		_mutex(&m), _bLocked(true)
	{
		m.lock();
	}
	UniLock::UniLock(Mutex& m, DeferLock_t):
		_mutex(&m), _bLocked(false)
	{}
	UniLock::UniLock(Mutex& m, AdoptLock_t):
		_mutex(&m), _bLocked(true)
	{}
	UniLock::UniLock(Mutex& m, TryLock_t):
		_mutex(&m), _bLocked(m.try_lock())
	{}
	UniLock::~UniLock() {
		unlock();
	}
	UniLock::UniLock(UniLock&& u): _mutex(u._mutex), _bLocked(u._bLocked) {
		u._mutex = nullptr;
		u._bLocked = false;
	}
	void UniLock::lock() {
		if(!_bLocked) {
			_mutex->lock();
			_bLocked = true;
		}
	}
	bool UniLock::tryLock() {
		if(!_bLocked)
			return _bLocked = _mutex->try_lock();
		return true;
	}
	void UniLock::unlock() {
		if(_bLocked) {
			_mutex->unlock();
			_bLocked = false;
		}
	}
	bool UniLock::isLocked() const {
		return _bLocked;
	}
	SDL_mutex* UniLock::getMutex() {
		if(_mutex)
			return _mutex->getMutex();
		return nullptr;
	}

	// ----- CondV -----
	CondV::CondV(): _cond(SDL_CreateCond()) {}
	CondV::~CondV() {
		SDL_DestroyCond(_cond);
	}
	void CondV::wait(UniLock& u) {
		SDLEC_P(Trap, SDL_CondWait, _cond, u.getMutex());
	}
	bool CondV::wait_for(UniLock& u, uint32_t msec) {
		return SDLEC_P(Trap, SDL_CondWaitTimeout, _cond, u.getMutex(), msec) == 0;
	}
	void CondV::signal() {
		SDLEC_P(Trap, SDL_CondSignal, _cond);
	}
	void CondV::signal_all() {
		SDLEC_P(Trap, SDL_CondBroadcast, _cond);
	}
}
