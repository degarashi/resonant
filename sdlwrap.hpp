#include <SDL2/SDL.h>
#include <string>
#include "spinner/misc.hpp"
#include <exception>
#include <stdexcept>

#define Assert(expr) AssertMsg(expr, "Assertion failed")
#define AssertMsg(expr, msg) { if(!(expr)) throw std::runtime_error(msg); }
#define SDLW_Check sdlw::CheckSDLError(__LINE__);
#ifdef DEBUG
	#define AAssert(expr) AAssertMsg(expr, "Assertion failed")
	#define AAssertMsg(expr, msg) AssertMsg(expr, msg)
	#define SDLW_ACheck sdlw::CheckSDLError(__LINE__);
#else
	#define AAssert(expr)
	#define AAssertMsg(expr,msg)
	#define SDLW_ACheck
#endif

namespace sdlw {
	extern SDL_threadID thread_local tls_threadID;
	void CheckSDLError(int line=-1);
	//! 実行環境に関する情報を取得
	class Spec : public spn::Singleton<Spec> {
		public:
			enum Feature : uint32_t {
				F_3DNow = 0x01,
				F_AltiVec = 0x02,
				F_MMX = 0x04,
				F_RDTSC = 0x08,
				F_SSE = 0x10,
				F_SSE2 = 0x11,
				F_SSE3 = 0x12,
				F_SSE41 = 0x14,
				F_SSE42 = 0x18
			};
			enum class PStatN {
				Unknown,
				OnBattery,
				NoBattery,
				Charging,
				Charged
			};
			struct PStat {
				PStatN	state;
				int		seconds,
				percentage;

				void output(std::ostream& os) const;
			};
		private:
			uint32_t		_feature;
			std::string		_platform;
			int				_nCacheLine,
			_nCpu;
		public:
			Spec();
			const std::string& getPlatform() const;

			int cpuCacheLineSize() const;
			int cpuCount() const;
			bool hasFuture(uint32_t flag) const;
			PStat powerStatus() const;
	};
	//! SDLのMutexラッパ
	class Mutex {
		SDL_mutex*	_mutex;

		public:
			Mutex();
			Mutex(const Mutex& m) = delete;
			Mutex& operator = (const Mutex& m) = delete;
			~Mutex();

			bool lock();
			bool try_lock();
			void unlock();
			SDL_mutex* getMutex();
	};
	//! 再帰対応のスピンロック
	template <class T>
	class SpinLock {
		struct Inner {
			SpinLock&	_src;
			T*			_data;

			Inner(const Inner&) = delete;
			Inner& operator = (const Inner&) = delete;
			Inner(SpinLock& src, T* data): _src(src), _data(data) { _src._lock(); }
			~Inner() {
				unlock();
			}
			T* operator -> () { return _data; }
			bool valid() const { return _data != nullptr; }
			void unlock() {
				if(_data) {
					_src._unlock();
					_data = nullptr;
				}
			}
		};
		SDL_atomic_t	_atmLock,
		_atmCount;
		void _unlock() {
			if(SDL_AtomicDecRef(&_atmCount) == SDL_TRUE)
				SDL_AtomicSet(&_atmLock, 0);
		}

		T	_data;
		Inner _lock(bool bBlock) {
			do {
				bool bSuccess = false;
				if(SDL_AtomicCAS(&_atmLock, 0, tls_threadID) == 0)
					bSuccess = true;
				else if(SDL_AtomicGet(&_atmLock) == tls_threadID) {
					// 同じスレッドからのロック
					bSuccess = true;
				}

				if(bSuccess) {
					// ロック成功
					SDL_AtomicAddRef(&_atmCount);
					return Inner(*this, &_data);
				}
			} while(bBlock);
			return Inner(*this, nullptr);
		}

		public:
			SpinLock() {
				SDL_AtomicSet(&_atmLock, 0);
				SDL_AtomicSet(&_atmCount, 0);
			}
			Inner lock() {
				return _lock(true);
			}
			Inner try_lock() {
				return _lock(false);
			}
	};

	// SIG = スレッドに関するシグニチャ
	template <class SIG>
	class Thread;
	template <class RET, class... Args>
	class Thread<RET (Args...)> {
		using This = Thread<RET (Args...)>;
		RET					_retVal;
		SDL_atomic_t		_atmInt;
		SDL_Thread*			_thread;
		std::exception_ptr	_eptr;

		enum Stat {
			Idle,			//!< スレッド開始前
			Running,		//!< スレッド実行中
			Interrupted,	//!< 中断要求がされたが、まだスレッドが終了していない
			Interrupted_End,//!< 中断要求によりスレッドが終了した
			Error_End,		//!< 例外による異常終了
			Finished		//!< 正常終了
		};
		SDL_atomic_t		_atmStat;
		SDL_cond			*_condC,		//!< 子スレッドが開始された事を示す
		*_condP;		//!< 親スレッドがクラス変数にスレッドポインタを格納した事を示す
		Mutex				_mtxC,
		_mtxP;

		static int ThreadFunc(void* p) {
			This* ths = reinterpret_cast<This*>(p);
			ths->_mtxC.lock();
			ths->_mtxP.lock();
			SDL_CondBroadcast(ths->_condC);
			ths->_mtxC.unlock();
			// 親スレッドが子スレッド変数をセットするまで待つ
			SDL_CondWait(ths->_condP, ths->_mtxP.getMutex());
			ths->_mtxP.unlock();

			// この時点でスレッドのポインタは有効な筈
			tls_threadID = SDL_GetThreadID(ths->_thread);
			Stat stat;
			try {
				ths->_retVal = ths->run();
				stat = (ths->isInterrupted()) ? Interrupted_End : Finished;
			} catch(...) {
				ths->_eptr = std::current_exception();
				stat = Error_End;
			}
			SDL_AtomicSet(&ths->_atmStat, stat);
			// SDLの戻り値は使わない
			return 0;
		}
		protected:
			virtual RET run(Args...) = 0;
		public:
			Thread(const Thread& t) = delete;
			Thread& operator = (const Thread& t) = delete;
			Thread(): _thread(nullptr), _eptr(nullptr) {
				_condC = SDL_CreateCond();
				_condP = SDL_CreateCond();
				// 中断フラグに0をセット
				SDL_AtomicSet(&_atmInt, 0);
				SDL_AtomicSet(&_atmStat, Idle);
			}
			~Thread() {
				// スレッド実行中だったらエラーを出す
				Assert(!isRunning())
				SDL_DestroyCond(_condC);
				SDL_DestroyCond(_condP);
			}
			template <class... Args0>
			void start(Args0&&... args0) {
				// 2回以上呼ぶとエラー
				Assert(SDL_AtomicGet(&_atmStat) == Idle)
				SDL_AtomicSet(&_atmStat, Running);

				_mtxC.lock();
				// 一旦クラス内部に変数を参照で取っておく
				SDL_Thread* th = SDL_CreateThread(ThreadFunc, "", this);
				// 子スレッドが開始されるまで待つ
				SDL_CondWait(_condC, _mtxC.getMutex());
				_mtxC.unlock();

				_mtxP.lock();
				_thread = th;
				SDL_CondBroadcast(_condP);
				_mtxP.unlock();
			}
			Stat getStatus() {
				return static_cast<Stat>(SDL_AtomicGet(&_atmStat));
			}
			bool isRunning() {
				Stat stat = getStatus();
				return stat==Running || stat==Interrupted;
			}
			//! 中断を指示する
			bool interrupt() {
				auto ret = SDL_AtomicCAS(&_atmStat, Running, Interrupted);
				SDL_AtomicSet(&_atmInt, 1);
				SDLW_ACheck
				return ret == SDL_TRUE;
			}
			//! スレッド内部で中断指示がされているかを確認
			bool isInterrupted() {
				return SDL_AtomicGet(&_atmInt) == 1;
			}
			void join() {
				_mtxP.lock();
				if(_thread) {
					int ret;
					SDL_WaitThread(_thread, &ret);
					_thread = nullptr;
				}
				_mtxP.unlock();
			}
			bool try_join(uint32_t ms) {
				_mtxP.lock();
				int res = SDL_CondWaitTimeout(_condP, _mtxP.getMutex(), ms);
				_mtxP.unlock();
				SDLW_Check
				return res == 0;
			}
			const RET& getResult() {
				// まだスレッドが終了して無い時の為にjoinを呼ぶ
				join();
				return _retVal;
			}
	};

	class Window;
	using SPWindow = std::shared_ptr<Window>;
	class GLContext;
	using SPGLContext = std::shared_ptr<GLContext>;

	class Window {
		public:
			enum class Stat {
				Minimized,
				Maximized,
				Hidden,
				Fullscreen,
				Shown
			};
		private:
			SDL_Window*	_window;
			Stat		_stat;
			Window(SDL_Window* w);

			void _checkState();
		public:
			static SPWindow CreateWindow(const std::string& title, int w, int h, uint32_t flag=0);
			static SPWindow CreateWindow(const std::string& title, int x, int y, int w, int h, uint32_t flag=0);
			~Window();

			void setFullscreen(bool bFull);
			void setGrab(bool bGrab);
			void setMaximumSize(int w, int h);
			void setMinimumSize(int w, int h);
			void setSize(int w, int h);
			void setTitle(const std::string& title);
			void show(bool bShow);
			void maximize();
			void minimize();
			void restore();
			void setPosition(int x, int y);
			void raise();
			// for logging
			uint32_t getID() const;

			Stat getState() const;
			bool isGrabbed() const;
			bool isResizable() const;
			bool hasInputFocus() const;
			bool hasMouseFocus() const;
			uint32_t getSDLFlag() const;
			SDL_Window* getWindow() const;

			static void EnableScreenSaver(bool bEnable);
	};
	class GLContext {
		SPWindow		_spWindow;
		SDL_GLContext	_ctx;

		GLContext(const SPWindow& w);
		public:
			static SPGLContext CreateContext(const SPWindow& w);
			~GLContext();
			void makeCurrent(const SPWindow& w);
			void makeCurrent();
			void swapWindow();
			static int SetSwapInterval(int n);
	};
}
