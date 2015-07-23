#pragma once
#include "spinner/misc.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include <SDL.h>
#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <SDL_events.h>
#include <SDL_image.h>
#include <exception>
#include <stdexcept>
#include "spinner/error.hpp"
#include "spinner/size.hpp"
#include <boost/serialization/access.hpp>
#include <boost/variant.hpp>
#include "spinner/ziptree.hpp"
#include "handle.hpp"
#include "sdlformat.hpp"

#define SDLEC_Base(flag, act, ...)		::spn::EChk_memory##flag<::spn::none_t>(AAct_##act<std::runtime_error>(), ::rs::SDLError(), SOURCEPOS, __VA_ARGS__)
#define SDLEC_Base0(flag, act)			::spn::EChk##flag(AAct_##act<std::runtime_error>(), ::rs::SDLError(), SOURCEPOS);
#define SDLEC(...)						SDLEC_Base(_a, __VA_ARGS__)
#define SDLEC_Chk(act)					SDLEC_Base0(_a, act)
#define SDLEC_D(...)					SDLEC_Base(_d, __VA_ARGS__)
#define SDLEC_Chk_D(act)				SDLEC_Base0(_d, act)

#define IMGEC_Base(flag, act, ...)		::spn::EChk_memory##flag<::spn::none_t>(AAct_##act<std::runtime_error>(), ::rs::IMGError(), SOURCEPOS, __VA_ARGS__)
#define IMGEC_Base0(flag, act)			::spn::EChk##flag(AAct_##act<std::runtime_error>(), ::rs::IMGError(), SOURCEPOS)
#define IMGEC(act, ...)					IMGEC_Base(_a, act, __VA_ARGS__)
#define IMGEC_Chk(act)					IMGEC_Base0(_a, act)
#define IMGEC_D(act, ...)				IMGEC_Base(_d, act, __VA_ARGS__)
#define IMGEC_Chk_D(act)				IMGEC_Base0(_d, act)

namespace rs {
	//! RAII形式でSDLの初期化 (for spn::MInitializer)
	struct SDLInitializer {
		SDLInitializer(uint32_t flag);
		~SDLInitializer();
	};
	struct IMGInitializer {
		IMGInitializer(uint32_t flag);
		~IMGInitializer();
	};

	struct SDLErrorI {
		static const char* Get();
		static void Reset();
		static const char *const c_apiName;
	};
	struct IMGErrorI {
		static const char* Get();
		static void Reset();
		static const char *const c_apiName;
	};

	template <class I>
	struct ErrorT {
		std::string		_errMsg;
		const char* errorDesc() {
			const char* err = I::Get();
			if(*err != '\0') {
				_errMsg = err;
				I::Reset();
				return _errMsg.c_str();
			}
			return nullptr;
		}
		void reset() const {
			I::Reset();
		}
		const char* getAPIName() const {
			return I::c_apiName;
		}
	};
	using SDLError = ErrorT<SDLErrorI>;
	using IMGError = ErrorT<IMGErrorI>;

	template <class T>
	class TLS;
	extern TLS<SDL_threadID>	tls_threadID;
	extern TLS<std::string>		tls_threadName;
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
			Mutex(Mutex&& m);
			Mutex(const Mutex&) = delete;
			Mutex& operator = (const Mutex&) = delete;
			~Mutex();

			bool lock();
			bool try_lock();
			void unlock();
			SDL_mutex* getMutex();
	};
	class UniLock {
		Mutex*	_mutex;
		bool	_bLocked;

		public:
			static struct DeferLock_t {} DeferLock;
			static struct AdoptLock_t {} AdoptLock;
			static struct TryLock_t {} TryLock;

			UniLock() = delete;
			UniLock(const UniLock&) = delete;
			void operator = (const UniLock& u) = delete;
			UniLock(Mutex& m, DeferLock_t);
			UniLock(Mutex& m, AdoptLock_t);
			UniLock(Mutex& m, TryLock_t);
			UniLock(Mutex& m);
			~UniLock();
			UniLock(UniLock&& u);
			void lock();
			void unlock();
			bool tryLock();
			bool isLocked() const;
			SDL_mutex* getMutex();
	};
	class CondV {
		SDL_cond*	_cond;
		public:
			CondV();
			~CondV();
			void wait(UniLock& u);
			bool wait_for(UniLock& u, uint32_t msec);
			void signal();
			void signal_all();
	};
	template <class SP, class T>
	struct SpinInner {
		SP&		_src;
		T*		_data;

		SpinInner(const SpinInner&) = delete;
		SpinInner& operator = (const SpinInner&) = delete;
		SpinInner(SpinInner&& n): _src(n._src), _data(n._data) {
			n._data = nullptr;
		}
		SpinInner(SP& src, T* data): _src(src), _data(data) {}
		~SpinInner() {
			unlock();
		}
		T& operator * () { return *_data; }
		T* operator -> () { return _data; }
		bool valid() const { return _data != nullptr; }
		void unlock() {
			if(_data) {
				_src._unlock();
				_data = nullptr;
			}
		}
		template <class T2>
		SpinInner<SP, T2> castAndMove() {
			SpinInner<SP, T2> ret(_src, reinterpret_cast<T2*>(_data));
			_data = nullptr;
			return ret;
		}
		template <class T2>
		SpinInner<SP, T2> castAndMoveDeRef() {
			SpinInner<SP, T2> ret(_src, reinterpret_cast<T2*>(*_data));
			_data = nullptr;
			return ret;
		}
	};

	//! thread local storage (with SDL)
	template <class T>
	class TLS {
		SDL_TLSID	_tlsID;
		static void Dtor(void* p) {
			delete reinterpret_cast<T*>(p);
		}
		T* _getPtr() {
			return reinterpret_cast<T*>(SDL_TLSGet(_tlsID));
		}
		const T* _getPtr() const {
			return reinterpret_cast<const T*>(SDL_TLSGet(_tlsID));
		}

		public:
			TLS() {
				_tlsID = SDLEC_D(Trap, SDL_TLSCreate);
			}
			template <class... Args>
			TLS(Args&&... args): TLS() {
				*this = T(std::forward<Args>(args)...);
			}
			template <class TA>
			TLS& operator = (TA&& t) {
				T* p = _getPtr();
				if(!p)
					SDL_TLSSet(_tlsID, new T(std::forward<TA>(t)), Dtor);
				else
					*p = std::forward<TA>(t);
				return *this;
			}
			bool valid() const {
				return _getPtr();
			}
			T& operator * () {
				return *_getPtr();
			}
			const T& operator * () const {
				return *_getPtr();
			}
			T& get() {
				return this->operator*();
			}
			const T& get() const {
				return this->operator*();
			}
			explicit operator bool() const {
				return _getPtr() != nullptr;
			}
			bool initialized() const {
				return _getPtr() != nullptr;
			}
			void terminate() {
				SDL_TLSSet(_tlsID, nullptr, nullptr);
			}
	};
	//! 再帰対応のスピンロック
	template <class T>
	class SpinLock {
		using Inner = SpinInner<SpinLock<T>, T>;
		using CInner = SpinInner<SpinLock<T>, const T>;
		friend Inner;
		friend CInner;

		SDL_atomic_t	_atmLock,
						_atmCount;
		void _unlock() {
			if(SDL_AtomicDecRef(&_atmCount) == SDL_TRUE)
				SDL_AtomicSet(&_atmLock, 0);
		}

		T	_data;
		template <class I>
		I _lock(bool bBlock) {
			do {
				bool bSuccess = false;
				if(SDL_AtomicCAS(&_atmLock, 0, *tls_threadID) == SDL_TRUE)
					bSuccess = true;
				else if(SDL_AtomicGet(&_atmLock) == static_cast<decltype(SDL_AtomicGet(&_atmLock))>(*tls_threadID)) {
					// 同じスレッドからのロック
					bSuccess = true;
				}

				if(bSuccess) {
					// ロック成功
					SDL_AtomicAdd(&_atmCount, 1);
					return I(*this, &_data);
				}
			} while(bBlock);
			return I(*this, nullptr);
		}

		public:
			SpinLock() {
				SDL_AtomicSet(&_atmLock, 0);
				SDL_AtomicSet(&_atmCount, 0);
			}
			Inner lock() {
				return _lock<Inner>(true);
			}
			CInner lockC() const {
				return const_cast<SpinLock*>(this)->_lock<CInner>(true);
			}
			Inner try_lock() {
				return _lock<Inner>(false);
			}
			CInner try_lockC() const {
				return const_cast<SpinLock*>(this)->_lock<CInner>(false);
			}
	};
	template <class T>
	class SpinLockP {
		struct InnerP {
			SpinLockP&	_s;
			bool		_bLocked;
			InnerP(InnerP&& p): _s(p._s), _bLocked(p._bLocked) {
				p._bLocked = false;
			}
			InnerP(SpinLockP& s): _s(s) {
				_bLocked = _s._put();
			}
			~InnerP() {
				if(_bLocked)
					_s._put_reset();
			}
		};
		using Inner = SpinInner<SpinLockP<T>, T>;
		using CInner = SpinInner<SpinLockP<T>, const T>;
		template <class T0, class T1>
		friend struct SpinInner;

		TLS<int>		_tlsCount;
		SDL_threadID	_lockID;
		int				_lockCount;
		Mutex			_mutex;
		T				_data;

		void _unlock() {
			AssertP(Trap, _lockID == *tls_threadID)
			AssertP(Trap, _lockCount >= 1)
			if(--_lockCount == 0)
				_lockID = 0;
			_mutex.unlock();
		}
		template <class I>
		I _lock(bool bBlock) {
			if(bBlock)
				_mutex.lock();
			if(bBlock || _mutex.try_lock()) {
				if(_lockID == 0) {
					_lockCount = 1;
					_lockID = *tls_threadID;
				} else {
					AssertP(Trap, _lockID == *tls_threadID)
					++_lockCount;
				}
				return I(*this, &_data);
			}
			return I(*this, nullptr);
		}
		void _put_reset() {
			_mutex.lock();
			_lockID = *tls_threadID;
			_lockCount = _tlsCount.get()-1;
			*_tlsCount = -1;

			int tmp = _lockCount;
			while(--tmp != 0)
				_mutex.lock();
		}
		bool _put() {
			// 自スレッドがロックしてたらカウンタを退避して一旦解放
			if(_mutex.try_lock()) {
				if(_lockCount > 0) {
					++_lockCount;
					*_tlsCount = _lockCount;
					int tmp = _lockCount;
					_lockCount = 0;
					_lockID = 0;
					while(tmp-- != 0)
						_mutex.unlock();
					return true;
				}
				_mutex.unlock();
			}
			return false;
		}

		public:
			SpinLockP(): _lockID(0), _lockCount(0) {
				_tlsCount = -1;
			}
			Inner lock() {
				return _lock<Inner>(true);
			}
			CInner lockC() const {
				return const_cast<SpinLockP*>(this)->_lock<CInner>(true);
			}
			Inner try_lock() {
				return _lock<Inner>(false);
			}
			CInner try_lockC() const {
				return const_cast<SpinLockP*>(this)->_lock<CInner>(false);
			}
			InnerP put() {
				return InnerP(*this);
			}
	};

	// SIG = スレッドに関するシグニチャ
	template <class DER, class RET, class... Args>
	class _Thread {
		using This = _Thread<DER, RET, Args...>;
		SDL_atomic_t		_atmInt;
		SDL_Thread*			_thread;

		using Holder = spn::ArgHolder<Args...>;
		using OPHolder = spn::Optional<Holder>;
		OPHolder			_holder;

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
		std::string			_name;

		static int ThreadFunc(void* p) {
			DER* ths = reinterpret_cast<DER*>(p);
			ths->_mtxC.lock();
			ths->_mtxP.lock();
			SDL_CondBroadcast(ths->_condC);
			ths->_mtxC.unlock();
			// 親スレッドが子スレッド変数をセットするまで待つ
			SDL_CondWait(ths->_condP, ths->_mtxP.getMutex());
			ths->_mtxP.unlock();

			// この時点でスレッドのポインタは有効な筈
			tls_threadID = SDL_GetThreadID(ths->_thread);
			tls_threadName = ths->_name;
			Stat stat;
			try {
				ths->_holder->inorder([ths](Args&&... args){ ths->runIt(std::forward<Args>(args)...); });
				stat = (ths->isInterrupted()) ? Interrupted_End : Finished;
			} catch(...) {
				Assert(Warn, false, "thread is finished unexpectedly")
#ifdef NO_EXCEPTION_PTR
				ths->_bException = true;
#else
				ths->_eptr = std::current_exception();
#endif
				stat = Error_End;
			}
			SDL_AtomicSet(&ths->_atmStat, stat);
			SDL_CondBroadcast(ths->_condP);
			// SDLの戻り値は使わない
			return 0;
		}
		//! move-ctorの後に値を掃除する
		void _clean() {
			_thread = nullptr;
			_holder = spn::none;
			_condC = _condP = nullptr;
		}
		protected:
#ifdef NO_EXCEPTION_PTR
			bool	_bException = false;
#else
			std::exception_ptr	_eptr = nullptr;
#endif
			virtual RET run(Args...) = 0;
		public:
			_Thread(_Thread&& th): _atmInt(std::move(th._atmInt)), _thread(th._thread), _holder(std::move(th._holder)),
				_atmStat(std::move(th._atmStat)), _condC(th._condC), _condP(th._condP),
				_mtxC(std::move(th._mtxC)), _mtxP(std::move(th._mtxP)), _name(std::move(th._name))
			{
				th._clean();
			}
			_Thread(const _Thread& t) = delete;
			_Thread& operator = (const _Thread& t) = delete;
			_Thread(const std::string& name): _thread(nullptr), _name(name) {
				_condC = SDL_CreateCond();
				_condP = SDL_CreateCond();
				// 中断フラグに0をセット
				SDL_AtomicSet(&_atmInt, 0);
				SDL_AtomicSet(&_atmStat, Idle);
			}
			~_Thread() {
				if(_thread) {
					// スレッド実行中だったらエラーを出す
					Assert(Trap, !isRunning())
					SDL_DestroyCond(_condC);
					SDL_DestroyCond(_condP);
				}
			}
			template <class... Args0>
			void start(Args0&&... args0) {
				_holder = Holder(std::forward<Args0>(args0)...);
				// 2回以上呼ぶとエラー
				Assert(Trap, SDL_AtomicGet(&_atmStat) == Idle)
				SDL_AtomicSet(&_atmStat, Running);

				_mtxC.lock();
				// 一旦クラス内部に変数を参照で取っておく
				SDL_Thread* th = SDL_CreateThread(ThreadFunc, _name.c_str(), this);
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
			virtual bool interrupt() {
				auto ret = SDL_AtomicCAS(&_atmStat, Running, Interrupted);
				SDL_AtomicSet(&_atmInt, 1);
				return ret == SDL_TRUE;
			}
			//! スレッド内部で中断指示がされているかを確認
			bool isInterrupted() {
				return SDL_AtomicGet(&_atmInt) == 1;
			}
			void join() {
				Assert(Trap, getStatus() != Stat::Idle)
				_mtxP.lock();
				if(_thread) {
					_mtxP.unlock();
					SDL_WaitThread(_thread, nullptr);
					_thread = nullptr;
					SDL_DestroyCond(_condP);
					SDL_DestroyCond(_condC);
					_condP = _condC = nullptr;
				} else
					_mtxP.unlock();
			}
			bool try_join(uint32_t ms) {
				Assert(Trap, getStatus() != Stat::Idle)
				_mtxP.lock();
				if(_thread) {
					int res = SDL_CondWaitTimeout(_condP, _mtxP.getMutex(), ms);
					_mtxP.unlock();
					SDLEC_Chk(Trap)
					if(res == 0 || !isRunning()) {
						SDL_WaitThread(_thread, nullptr);
						_thread = nullptr;
						return true;
					}
					return false;
				}
				_mtxP.unlock();
				return true;
			}
			void getResult() {
				// まだスレッドが終了して無い時の為にjoinを呼ぶ
				join();
				rethrowIfException();
			}
			void rethrowIfException() {
#ifdef NO_EXCEPTION_PTR
				if(_bException)
					throw std::runtime_error("exception catched in thread");
#else
				if(_eptr)
					std::rethrow_exception(_eptr);
#endif
			}
			const std::string& getName() const {
				return _name;
			}
	};
	template <class SIG>
	class Thread;
	template <class RET, class... Args>
	class Thread<RET (Args...)> : public _Thread<Thread<RET (Args...)>, RET, Args...> {
		using base_type = _Thread<Thread<RET (Args...)>, RET, Args...>;
		spn::Optional<RET> 	_retVal;
		protected:
			virtual RET run(Args...) = 0;
		public:
			using base_type::base_type;
			void runIt(Args... args) {
				_retVal = run(std::forward<Args>(args)...);
			}
			RET&& getResult() {
				// まだスレッドが終了して無い時の為にjoinを呼ぶ
				base_type::join();
				base_type::rethrowIfException();
				return std::move(*_retVal);
			}
	};
	template <class... Args>
	class Thread<void (Args...)> : public _Thread<Thread<void (Args...)>, void, Args...> {
		using base_type = _Thread<Thread<void (Args...)>, void, Args...>;
		protected:
			virtual void run(Args...) = 0;
		public:
			using base_type::base_type;
			void runIt(Args... args) {
				run(std::forward<Args>(args)...);
			}
			void getResult() {
				// まだスレッドが終了して無い時の為にjoinを呼ぶ
				base_type::join();
				base_type::rethrowIfException();
			}
	};

	class Window;
	using SPWindow = std::shared_ptr<Window>;

	class Window {
		public:
			enum class Stat {
				Minimized,
				Maximized,
				Hidden,
				Fullscreen,
				Shown
			};
			//! OpenGL初期化パラメータ
			struct GLParam {
				int			verMajor, verMinor;			//!< OpenGLバージョン(メジャー & マイナー)
				int			red, green, blue, depth;	//!< 色深度(RGB + Depth)
				int			doublebuffer;				//!< ダブルバッファフラグ

				GLParam();
				void setStdAttributes() const;
				void getStdAttributes();
			};
			//! Window初期化パラメータ
			struct Param {
				std::string	title;						//!< ウィンドウタイトル
				int			posx, posy,					//!< ウィンドウ位置
							width, height;				//!< ウィンドウサイズ
				uint32_t	flag;						//!< その他のフラグ

				Param();
			};
		private:
			SDL_Window*	_window;
			Stat		_stat;
			Window(SDL_Window* w);

			void _checkState();
		public:
			static void SetGLAttributes() {}
			// SDL_GLパラメータ設定(可変引数バージョン)
			template <class... Ts>
			static void SetGLAttributes(SDL_GLattr attr, int value, Ts... ts) {
				SDL_GL_SetAttribute(attr, value);
				SetGLAttributes(ts...);
			}
			static void GetGLAttributes() {}
			// SDL_GLパラメータ取得(可変引数バージョン)
			template <class... Ts>
			static void GetGLAttributes(SDL_GLattr attr, int& dst, Ts&&... ts) {
				SDL_GL_GetAttribute(attr, &dst);
				GetGLAttributes(std::forward<Ts>(ts)...);
			}

			static SPWindow Create(const Param& p);
			static SPWindow Create(const std::string& title, int w, int h, uint32_t flag=0);
			static SPWindow Create(const std::string& title, int x, int y, int w, int h, uint32_t flag=0);
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
			spn::Size getSize() const;
			spn::Size getMaximumSize() const;
			spn::Size getMinimumSize() const;
			uint32_t getSDLFlag() const;
			SDL_Window* getWindow() const;

			static void EnableScreenSaver(bool bEnable);
	};

	class GLContext;
	using SPGLContext = std::shared_ptr<GLContext>;
	class GLContext {
		//! makeCurrentした時に指定したウィンドウ
		SPWindow		_spWindow;
		SDL_GLContext	_ctx;

		GLContext(const SPWindow& w);
		public:
			static SPGLContext CreateContext(const SPWindow& w, bool bShare=false);
			~GLContext();
			void makeCurrent(const SPWindow& w);
			void makeCurrent();
			void swapWindow();
			static int SetSwapInterval(int n);
	};

	class RWops {
		public:
			struct Callback : boost::serialization::traits<Callback,
						boost::serialization::object_serializable,
						boost::serialization::track_never>
			{
				virtual void onRelease(RWops& rw) = 0;
				template <class Archive>
				void serialize(Archive&, const unsigned int) {}
			};
			enum Hence : int {
				Begin = RW_SEEK_SET,
				Current = RW_SEEK_CUR,
				End = RW_SEEK_END
			};
			// 管轄外メモリであってもシリアライズ時にはデータ保存する
			enum Type {
				ConstMem,		//!< 外部Constメモリ	(管轄外)
				Mem,			//!< 外部メモリ			(管轄外)
				ConstVector,	//!< 管轄Constメモリ
				Vector,			//!< 管轄メモリ
				File
			};
			enum Access : int {
				Read = 0x01,
				Write = 0x02,
				Binary = 0x04
			};
			//! 外部バッファ
			struct ExtBuff {
				void* 	ptr;
				size_t	size;

				template <class Archive>
				void serialize(Archive& /*ar*/, const unsigned int /*ver*/) {
					AssertP(Trap, false, "this object cannot serialize")
				}
			};

			//! RWopsエラー基底
			struct RWE_Error : std::runtime_error {
				const std::string	_title;

				RWE_Error(const std::string& title);
				void setMessage(const std::string& msg);
			};
			//! ファイル開けないエラー
			struct RWE_File : RWE_Error {
				const std::string	_path;

				RWE_File(const std::string& path);
			};
			//! 範囲外アクセス
			struct RWE_OutOfRange : RWE_Error {
				const Hence		_hence;
				const int64_t	_pos, _size;

				RWE_OutOfRange(int64_t pos, Hence hence, int64_t size);
			};
			//! アクセス権限違反(読み取り専用ファイルに書き込みなど)
			struct RWE_Permission : RWE_Error {
				const Access	_have, _try;

				RWE_Permission(Access have, Access tr);
			};
			//! メモリ確保エラー
			struct RWE_Memory : RWE_Error {
				const size_t	_size;

				RWE_Memory(size_t size);
			};
			//! ぬるぽ指定
			struct RWE_NullMemory : RWE_Error {
				RWE_NullMemory();
			};

		private:
			using Data = boost::variant<boost::blank, std::string, spn::URI, ExtBuff, spn::ByteBuff>;
			SDL_RWops*	_ops;
			int			_access;
			Type		_type;
			Data		_data;

			//! RWopsが解放される直前に呼ばれる関数
			using UPCallback = std::unique_ptr<Callback>;
			UPCallback	_endCB;

			void _deserializeFromMember();
			void _clear();
			friend class boost::serialization::access;
			BOOST_SERIALIZATION_SPLIT_MEMBER();
			template <class Archive>
			void save(Archive& ar, const unsigned int) const {
				ar & _access;
				if(isMemory()) {
					Type type;
					if(_type == Type::ConstMem || _type == Type::ConstVector)
						type = Type::ConstVector;
					else
						type = Type::Vector;
					ar & type;

					auto dat = getMemoryPtrC();
					spn::ByteBuff buff(dat.second);
					std::memcpy(&buff[0], dat.first, dat.second);
					Data data(std::move(buff));
					ar & data;
				} else
					ar & _type & _data;
				int64_t pos = tell();
				ar & pos & _endCB;
			}
			template <class Archive>
			void load(Archive& ar, const unsigned int) {
				int64_t pos;
				ar & _access & _type & _data & pos & _endCB;
				_deserializeFromType(pos);
			}

			void _deserializeFromType(int64_t pos);
			friend spn::ResWrap<RWops, spn::String<std::string>>;
			RWops() = default;
			template <class T>
			RWops(SDL_RWops* ops, Type type, int access, T&& data, Callback* cb=nullptr) {
				AssertT(Trap, ops, (RWE_Error)(const char*), "unknown error")
				_ops = ops;
				_type = type;
				_access = access;
				_data = std::forward<T>(data);
				_endCB.reset(cb);
			}
			static RWops _FromVector(spn::ByteBuff&& buff, Callback* cb, std::false_type);
			static RWops _FromVector(spn::ByteBuff&& buff, Callback* cb, std::true_type);

		public:
			static int ReadMode(const char* mode);
			static std::string ReadModeStr(int mode);

			static RWops FromConstMem(const void* mem, size_t size, Callback* cb=nullptr);
			template <class T>
			static RWops FromVector(T&& buff, Callback* cb=nullptr) {
				spn::ByteBuff tbuff(std::forward<T>(buff));
				return _FromVector(std::move(tbuff), cb, typename std::is_const<T>::type());
			}
			static RWops FromMem(void* mem, size_t size, Callback* cb=nullptr);
			static RWops FromFile(const std::string& path, int access);
			static RWops FromURI(SDL_RWops* ops, const spn::URI& uri, int access);

			RWops(RWops&& ops);
			RWops& operator = (RWops&& ops);
			~RWops();

			void close();
			int getAccessFlag() const;
			size_t read(void* dst, size_t blockSize, size_t nblock);
			size_t write(const void* src, size_t blockSize, size_t nblock);
			int64_t size();
			int64_t seek(int64_t offset, Hence hence);
			int64_t tell() const;
			uint16_t readBE16();
			uint32_t readBE32();
			uint64_t readBE64();
			uint16_t readLE16();
			uint32_t readLE32();
			uint64_t readLE64();
			bool writeBE(uint16_t value);
			bool writeBE(uint32_t value);
			bool writeBE(uint64_t value);
			bool writeLE(uint16_t value);
			bool writeLE(uint32_t value);
			bool writeLE(uint64_t value);
			SDL_RWops* getOps();
			spn::ByteBuff readAll();
			Type getType() const;
			bool isMemory() const;
			//! isMemory()==trueの時だけ有効
			std::pair<const void*, size_t> getMemoryPtrC() const;
			//! Type::Vector時のみ有効
			std::pair<void*, size_t> getMemoryPtr();
	};
	struct UriHandler : spn::IURIOpener {
		virtual HLRW openURI_RW(const spn::URI& uri, int access) = 0;
	};
	using SPUriHandler = std::shared_ptr<UriHandler>;

	#define mgr_rw (::rs::RWMgr::_ref())
	class RWMgr : public spn::ResMgrA<RWops, RWMgr> {
		struct HChk {
			HLRW operator()(UriHandler& h, const spn::URI& uri, int access) const;
		};
		using UriHandlerV = spn::HandlerV<UriHandler, HChk>;
		UriHandlerV		_handlerV;
		std::string		_orgName,
						_appName;
		//! 一時ファイルディレクトリのファイルを全て削除
		void _cleanupTemporaryFile();

		public:
			RWMgr(const std::string& org_name, const std::string& app_name);
			UriHandlerV& getHandler();
			using base_type = spn::ResMgrA<RWops, RWMgr>;
			using LHdl = AnotherLHandle<RWops, true>;

			// ---- RWopsへ中継するだけの関数 ----
			//! 任意のURIからハンドル作成(ReadOnly)
			LHdl fromURI(const spn::URI& uri, int access);
			LHdl fromFile(const std::string& path, int access);
			template <class T>
			LHdl fromVector(T&& t) {
				return base_type::acquire(RWops::FromVector(std::forward<T>(t))); }
			LHdl fromConstMem(const void* p, int size, typename RWops::Callback* cb=nullptr);
			LHdl fromMem(void* p, int size, typename RWops::Callback* cb=nullptr);
			//! ランダムな名前の一時ファイルを作ってそのハンドルとファイルパスを返す
			std::pair<LHdl, std::string> createTemporaryFile();
			//! OrgNameとAppNameからなるプライベートなディレクトリパス
			std::string makeFilePath(const std::string& dirName) const;
	};
	//! ファイルシステムに置かれたZipからのファイル読み込み
	class UriH_PackedZip : public UriHandler {
		spn::UP_Adapt		_stream;
		spn::zip::ZipTree	_ztree;
		static bool Capable(const spn::URI& uri);
		public:
			UriH_PackedZip(spn::ToPathStr zippath);
			// --- from UriHandler ---
			HLRW openURI_RW(const spn::URI& uri, int access) override;
			// --- from IURIOpener ---
			spn::UP_Adapt openURI(const spn::URI& uri) override;
	};
	//! アセットZipからのファイル読み込み (Android only)
	class UriH_AssetZip : public UriH_PackedZip {
		public:
			// Asset中のZipファイルのパスを指定
			UriH_AssetZip(spn::ToPathStr zippath);
	};
	//! ファイルシステムからのファイル読み込み
	class UriH_File : public UriHandler {
		spn::PathStr	_basePath;
		public:
			UriH_File(spn::ToPathStr path);
			static bool Capable(const spn::URI& uri, int access);
			// --- from UriHandler ---
			HLRW openURI_RW(const spn::URI& uri, int access) override;
			// --- from IURIOpener ---
			spn::UP_Adapt openURI(const spn::URI& uri) override;
	};

	struct RGB {
		union {
			uint8_t ar[3];
			struct { uint8_t r,g,b; };
		};
		RGB() = default;
		RGB(int r, int g, int b): ar{uint8_t(r), uint8_t(g), uint8_t(b)} {}
	};
	struct RGBA {
		union {
			uint8_t ar[4];
			struct { uint8_t r,g,b,a; };
		};
		RGBA() = default;
		RGBA(RGB rgb, int a): ar{rgb.r, rgb.g, rgb.b, static_cast<uint8_t>(a)} {}
	};

	using UPSDLFormat = std::unique_ptr<SDL_PixelFormat, decltype(&SDL_FreeFormat)>;
	UPSDLFormat MakeUPFormat(uint32_t fmt);

	class Surface;
	using SPSurface = std::shared_ptr<Surface>;
	class Surface {
		SDL_Surface*	_sfc;
		mutable Mutex	_mutex;
		spn::AB_Byte	_buff;
		class LockObj {
			const Surface& 	_sfc;
			void*		_bits;
			int			_pitch;

			public:
				LockObj(LockObj&& lk);
				LockObj(const Surface& sfc, void* bits, int pitch);
				~LockObj();
				void* getBits();
				int getPitch() const;
				operator bool () const;
		};
		void _unlock() const;

		Surface(SDL_Surface* sfc);
		Surface(SDL_Surface* sfc, spn::ByteBuff&& buff);
		public:
			static uint32_t Map(uint32_t format, RGB rgb);
			//! RGBA値をSDLのピクセルフォーマット形式にパックする
			static uint32_t Map(uint32_t format, RGBA rgba);
			//! SDLのピクセルフォーマットからRGB値を取り出す
			static RGBA Get(uint32_t format, uint32_t pixel);
			//! SDLのピクセルフォーマット名を表す文字列を取得
			static const std::string& GetFormatString(uint32_t format);

			//! 任意のフォーマットの画像を読み込む
			static SPSurface Load(HRW hRW);
			//! 空のサーフェス作成
			static SPSurface Create(int w, int h, uint32_t format);
			//! ピクセルデータを元にサーフェス作成
			static SPSurface Create(const spn::ByteBuff& src, int pitch, int w, int h, uint32_t format);
			static SPSurface Create(spn::ByteBuff&& src, int pitch, int w, int h, uint32_t format);

			~Surface();
			void saveAsBMP(HRW hDst) const;
			void saveAsPNG(HRW hDst) const;
			LockObj lock() const;
			LockObj try_lock() const;
			const SDL_PixelFormat& getFormat() const;
			uint32_t getFormatEnum() const;
			spn::Size getSize() const;
			int width() const;
			int height() const;
			//! 同サイズのサーフェスを作成
			SPSurface makeBlank() const;
			SPSurface duplicate() const;
			SPSurface flipHorizontal() const;
			SPSurface flipVertical() const;
			//! ピクセルフォーマット変換
			SPSurface convert(uint32_t fmt) const;
			SPSurface convert(const SDL_PixelFormat& fmt) const;
			//! ピクセルデータがデータ配列先頭から隙間なく詰められているか
			bool isContinuous() const;
			//! Continuousな状態でピクセルデータを抽出
			spn::ByteBuff extractAsContinuous(uint32_t dstFmt=0) const;
			//! ビットブロック転送
			void blit(const SPSurface& sfc, const spn::Rect& srcRect, int dstX, int dstY) const;
			//! スケーリング有りのビットブロック転送
			void blitScaled(const SPSurface& sfc, const spn::Rect& srcRect, const spn::Rect& dstRect) const;
			//! 単色での矩形塗りつぶし
			void fillRect(const spn::Rect& rect, uint32_t color);
			SDL_Surface* getSurface();
			SPSurface resize(const spn::Size& s) const;
			void setEnableColorKey(uint32_t key);
			void setDisableColorKey();
			spn::Optional<uint32_t> getColorKey() const;
			void setBlendMode(SDL_BlendMode mode);
			SDL_BlendMode getBlendMode() const;
	};
}
