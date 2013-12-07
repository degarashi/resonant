#pragma once
#include "spinner/misc.hpp"
#include "spinner/resmgr.hpp"
#include "spinner/dir.hpp"
#include <SDL.h>
#include <SDL_atomic.h>
#include <SDL_thread.h>
#include <SDL_events.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <exception>
#include <stdexcept>
#include <boost/optional.hpp>
#include "spinner/error.hpp"
#include "spinner/size.hpp"

#define SDLEC_Base(act, ...)	::spn::EChk_base(act, SDLError(), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define SDLEC_Base0(act)		::spn::EChk_base(act, SDLError(), __FILE__, __PRETTY_FUNCTION__, __LINE__);
#define SDLEC(act, ...)			SDLEC_Base(AAct_##act<std::runtime_error>(), __VA_ARGS__)
#define SDLEC_Chk(act)			SDLEC_Base0(AAct_##act<std::runtime_error>())

#define IMGEC_Base(act, ...)	::spn::EChk_base(act, IMGError(), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define IMGEC_Base0(act)		::spn::EChk_base(act, IMGError(), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define IMGEC(act, ...)			IMGEC_Base(AAct_##act<std::runtime_error>(), __VA_ARGS__)
#define IMGEC_Chk(act)			IMGEC_Base0(AAct_##act<std::runtime_error>())

#define TTFEC_Base(act, ...)	::spn::EChk_base(act, TTFError(), __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define TTFEC_Base0(act)		::spn::EChk_base(act, TTFError(), __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define TTFEC(act, ...)			TTFEC_Base(AAct_##act<std::runtime_error>(), __VA_ARGS__)
#define TTFEC_Chk(act)			TTFEC_Base0(AAct_##act<std::runtime_error>())

#ifdef DEBUG
	#define SDLEC_P(act, ...)	SDLEC(act, __VA_ARGS__)
	#define SDLEC_ChkP(act)		SDLEC_Chk(act)
	#define IMGEC_P(act, ...)	IMGEC(act, __VA_ARGS__)
	#define IMGEC_ChkP(act)		IMGEC_Chk(act)
	#define TTFEC_P(act, ...)	TTFEC(act, __VA_ARGS__)
	#define TTFEC_ChkP(act)		TTFEC_Chk(act)
#else
	#define SDLEC_P(act, ...)	::spn::EChk_pass(__VA_ARGS__)
	#define SDLEC_ChkP(act)
	#define IMGEC_P(act, ...)	::spn::EChk_pass(__VA_ARGS__)
	#define IMGEC_ChkP(act)
	#define TTFEC_P(act, ...)	::spn::EChk_pass(__VA_ARGS__)
	#define TTFEC_ChkP(act)
#endif

namespace rs {
	//! RAII形式でSDLの初期化 (for spn::MInitializer)
	struct SDLInitializer {
		SDLInitializer(uint32_t flag);
		~SDLInitializer();
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
	struct TTFErrorI {
		static const char* Get();
		static void Reset();
		static const char *const c_apiName;
	};
	template <class I>
	struct ErrorT {
		const char* errorDesc() const {
			const char* err = I::Get();
			if(*err != '\0') {
				spn::tls_errMsgTmp = err;
				I::Reset();
				return spn::tls_errMsgTmp.c_str();
			}
			return nullptr;
		}
		void reset() const {
			I::Reset();
		}
		const char *const getAPIName() const {
			return I::c_apiName;
		}
	};
	using SDLError = ErrorT<SDLErrorI>;
	using IMGError = ErrorT<IMGErrorI>;
	using TTFError = ErrorT<TTFErrorI>;

	extern SDL_threadID thread_local tls_threadID;
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
		Mutex* _mutex;
		public:
			UniLock() = delete;
			UniLock(const UniLock&) = delete;
			void operator = (const UniLock& u) = delete;
			UniLock(Mutex& m): _mutex(&m) { m.lock(); }
			~UniLock() {
				unlock();
			}
			UniLock(UniLock&& u): _mutex(u._mutex) {
				u._mutex = nullptr;
			}
			void unlock() {
				if(_mutex) {
					_mutex->unlock();
					_mutex = nullptr;
				}
			}
			SDL_mutex* getMutex() {
				if(_mutex)
					return _mutex->getMutex();
				return nullptr;
			}
	};
	class CondV {
		SDL_cond*	_cond;
		public:
			CondV(): _cond(SDL_CreateCond()) {}
			~CondV() {
				SDL_DestroyCond(_cond);
			}
			void wait(UniLock& u) {
				SDLEC_P(Trap, SDL_CondWait, _cond, u.getMutex());
			}
			bool wait_for(UniLock& u, uint32_t msec) {
				return SDLEC_P(Trap, SDL_CondWaitTimeout, _cond, u.getMutex(), msec) == 0;
			}
			void signal() {
				SDLEC_P(Trap, SDL_CondSignal, _cond);
			}
			void signal_all() {
				SDLEC_P(Trap, SDL_CondBroadcast, _cond);
			}
	};
	template <class SP, class T>
	struct SpinInner {
		SP&		_src;
		T*		_data;

		SpinInner(const SpinInner&) = delete;
		SpinInner& operator = (const SpinInner&) = delete;
		SpinInner(SpinInner&& n): _src(n._src), _data(n._data) {}
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
				if(SDL_AtomicCAS(&_atmLock, 0, tls_threadID) == SDL_TRUE)
					bSuccess = true;
				else if(SDL_AtomicGet(&_atmLock) == tls_threadID) {
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
				_tlsID = SDLEC_P(Trap, SDL_TLSCreate);
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
	};

	// SIG = スレッドに関するシグニチャ
	template <class DER, class RET, class... Args>
	class _Thread {
		using This = _Thread<DER, RET, Args...>;
		SDL_atomic_t		_atmInt;
		SDL_Thread*			_thread;

		using Holder = spn::ArgHolder<Args...>;
		spn::Optional<Holder> _holder;

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
			Stat stat;
			try {
				ths->_holder->inorder([ths](Args... args){ ths->runIt(std::forward<Args>(args)...); });
				stat = (ths->isInterrupted()) ? Interrupted_End : Finished;
			} catch(...) {
				ths->_eptr = std::current_exception();
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
			std::exception_ptr	_eptr;
			virtual RET run(Args...) = 0;
		public:
			_Thread(_Thread&& th): _atmInt(std::move(th._atmInt)), _thread(th._thread), _holder(std::move(th._holder)),
				_atmStat(std::move(th._atmStat)), _condC(th._condC), _condP(th._condP),
				_mtxC(std::move(th._mtxC)), _mtxP(std::move(th._mtxP))
			{
				th._clean();
			}
			_Thread(const _Thread& t) = delete;
			_Thread& operator = (const _Thread& t) = delete;
			_Thread(): _thread(nullptr), _eptr(nullptr) {
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
				if(_eptr)
					std::rethrow_exception(_eptr);
			}
	};
	template <class SIG>
	class Thread;
	template <class RET, class... Args>
	class Thread<RET (Args...)> : public _Thread<Thread<RET (Args...)>, RET, Args...> {
		using base_type = _Thread<Thread<RET (Args...)>, RET, Args...>;
		boost::optional<RET> 	_retVal;
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
				if(base_type::_eptr)
					std::rethrow_exception(base_type::_eptr);
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
				if(base_type::_eptr)
					std::rethrow_exception(base_type::_eptr);
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
		private:
			SDL_Window*	_window;
			Stat		_stat;
			Window(SDL_Window* w);

			void _checkState();
		public:
			template <class... Ts>
			static void SetGLAttributes(Ts... ts) {}
			template <class... Ts>
			static void SetGLAttributes(SDL_GLattr attr, int value, Ts... ts) {
				SDL_GL_SetAttribute(attr, value);
				SetGLAttributes(ts...);
			}
			static void SetStdGLAttributes(int major, int minor, int depth);
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
			enum Hence : int {
				Begin = RW_SEEK_SET,
				Current = RW_SEEK_CUR,
				End = RW_SEEK_END
			};
			enum Type {
				ConstMem,		//!< 外部Constメモリ	(管轄外)
				Mem,			//!< 外部メモリ			(管轄外)
				Vector,			//!< 管轄メモリ
				File,
				FilePointer
			};
			using EndCB = std::function<void (RWops&)>;
		private:
			SDL_RWops*	_ops;
			int			_access;
			Type		_type;
			const void*	_ptr;
			size_t		_size;
			enum Access : int {
				Read = 0x01,
				Write = 0x02,
				Binary = 0x04
			};
			//! RWopsが解放される直前に呼ばれる関数
			EndCB		_endCB;

			void _clear();
			RWops(SDL_RWops* ops, int access, Type type, EndCB cb, const void* ptr=nullptr, size_t sz=0);
		public:
			static int ReadMode(const char* mode);
			static std::string ReadModeStr(int mode);
			static RWops FromConstMem(const void* mem, int size, EndCB cb=nullptr);
			template <class T>
			static RWops FromVector(T&& buff) {
				spn::ByteBuff* tbuff = new spn::ByteBuff(std::forward<T>(buff));
				return RWops(SDL_RWFromMem(&(*tbuff)[0], tbuff->size()), Read|Write, Type::Vector, [tbuff](RWops&){
					delete tbuff;
				}, &(*tbuff)[0], tbuff->size());
			}
			static RWops FromFilePointer(FILE* fp, bool autoClose, const char* mode);
			static RWops FromMem(void* mem, int size, EndCB cb=nullptr);
			static RWops FromFile(spn::ToPathStr path, const char* mode);

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
	struct UriHandler;
	using SPUriHandler = std::shared_ptr<UriHandler>;
	#define mgr_rw (::rs::RWMgr::_ref())
	class RWMgr : public spn::ResMgrN<RWops, RWMgr> {
		using HandlerL = std::vector<SPUriHandler>;
		HandlerL _handler;

		public:
			using base_type = spn::ResMgrN<RWops, RWMgr>;
			using LHdl = AnotherLHandle<RWops>;
			void addUriHandler(const SPUriHandler& h);
			void remUriHandler(const SPUriHandler& h);

			//! 任意のURIからハンドル作成(ReadOnly)
			/*! \param[in] bNoShared trueなら同じキーでリソースを共有しない */
			LHdl fromURI(const spn::URI& uri, const char* mode, bool bNoShared);
			LHdl fromFile(spn::ToPathStr path, const char* mode, bool bNoShared);
			template <class T>
			LHdl fromVector(T&& t) {
				return base_type::acquire(RWops::FromVector(std::forward<T>(t))); }
			LHdl fromConstMem(const void* p, int size, typename RWops::EndCB cb=nullptr);
			LHdl fromMem(void* p, int size, typename RWops::EndCB cb=nullptr);
			LHdl fromFP(FILE* fp, bool bAutoClose, const char* mode);
	};
	DEF_HANDLE(RWMgr, RW, RWops)
	struct UriHandler {
		virtual HLRW loadURI(const spn::URI& uri, int access, bool bNoShared) = 0;
	};
	//! アセットZipからのファイル読み込み (Android only)
	struct UriH_AssetZip : UriHandler {
		spn::PathStr	_zipPath;			//!< Asset中のZipファイルのパス
		UriH_AssetZip(spn::ToPathStr zippath);
		HLRW loadURI(const spn::URI& uri, int access, bool bNoShared) override;
	};
	//! ファイルシステムからのファイル読み込み
	struct UriH_File : UriHandler {
		spn::PathStr	_basePath;
		UriH_File(spn::ToPathStr path);
		HLRW loadURI(const spn::URI& uri, int access, bool bNoShared) override;
	};

	struct RGB {
		union {
			uint8_t ar[3];
			struct { uint8_t r,g,b; };
		};
	};
	struct RGBA {
		union {
			uint8_t ar[4];
			struct { uint8_t r,g,b,a; };
		};
		RGBA() = default;
		RGBA(RGB rgb, int a): ar{rgb.r, rgb.g, rgb.b, static_cast<uint8_t>(a)} {}
	};

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
			static uint32_t Map(uint32_t format, RGBA rgba);
			static RGBA Get(uint32_t format, uint32_t pixel);

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
			void fillRect(const spn::Rect& rect, uint32_t color);
			LockObj lock() const;
			LockObj try_lock() const;
			spn::Size getSize() const;
			const SDL_PixelFormat& getFormat() const;
			int width() const;
			int height() const;
			SPSurface convert(uint32_t fmt) const;
			SPSurface convert(const SDL_PixelFormat& fmt) const;
			bool isContinuous() const;
			spn::ByteBuff extractAsContinuous(uint32_t dstFmt=0) const;
			void blit(const SPSurface& sfc, const spn::Rect& srcRect, int dstX, int dstY) const;
			void blitScaled(const SPSurface& sfc, const spn::Rect& srcRect, const spn::Rect& dstRect) const;
			SDL_Surface* getSurface();
			SPSurface resize(const spn::Size& s) const;
	};
}
