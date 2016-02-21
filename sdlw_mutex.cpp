#include "sdlwrap.hpp"

namespace rs {
	// ----- Mutex -----
	Mutex::Mutex(): _mutex(SDL_CreateMutex()) {}
	Mutex::Mutex(Mutex&& m): _mutex(m._mutex) {
		m._mutex = nullptr;
	}
	Mutex::~Mutex() {
		if(_mutex)
			SDLEC_D(Warn, SDL_DestroyMutex, _mutex);
	}
	bool Mutex::lock() {
		return SDLEC_D(Warn, SDL_LockMutex, _mutex) == 0;
	}
	bool Mutex::try_lock() {
		// 単に他がロックしている時もTryLockが失敗するのでSDL_GetErrorによるエラーチェックはしない
		auto res = SDL_TryLockMutex(_mutex);
		// TryLockが失敗した時に後のエラーチェックに響くため、ここでリセット
		SDL_ClearError();
		return res == 0;
	}
	void Mutex::unlock() {
		SDLEC_D(Warn, SDL_UnlockMutex, _mutex);
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
	UniLock::operator bool() const {
		return isLocked();
	}

	// ----- CondV -----
	CondV::CondV(): _cond(SDL_CreateCond()) {}
	CondV::~CondV() {
		SDL_DestroyCond(_cond);
	}
	void CondV::wait(UniLock& u) {
		SDLEC_D(Trap, SDL_CondWait, _cond, u.getMutex());
	}
	bool CondV::wait_for(UniLock& u, uint32_t msec) {
		return SDLEC_D(Trap, SDL_CondWaitTimeout, _cond, u.getMutex(), msec) == 0;
	}
	void CondV::signal() {
		SDLEC_D(Trap, SDL_CondSignal, _cond);
	}
	void CondV::signal_all() {
		SDLEC_D(Trap, SDL_CondBroadcast, _cond);
	}
}
