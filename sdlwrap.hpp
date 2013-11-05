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
#include "error.hpp"
#include "spinner/size.hpp"

#define SDLEC_Base(act, ...)	EChk_base<SDLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define SDLEC_Base0(act)		EChk_base<SDLError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__);
#define SDLEC(act, ...)			SDLEC_Base(AAct_##act<std::runtime_error>(), __VA_ARGS__)
#define SDLEC_Chk(act)			SDLEC_Base0(AAct_##act<std::runtime_error>())

#define IMGEC_Base(act, ...)	EChk_base<IMGError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define IMGEC_Base0(act)		EChk_base<IMGError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__)
#define IMGEC(act, ...)			IMGEC_Base(AAct_##act<std::runtime_error>(), __VA_ARGS__)
#define IMGEC_Chk(act)			IMGEC_Base0(AAct_##act<std::runtime_error>())

#define TTFEC_Base(act, ...)	EChk_base<TTFError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__, __VA_ARGS__)
#define TTFEC_Base0(act)		EChk_base<TTFError>(act, __FILE__, __PRETTY_FUNCTION__, __LINE__)
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
	#define SDLEC_P(act, ...)	EChk_pass(__VA_ARGS__)
	#define SDLEC_ChkP(act)
	#define IMGEC_P(act, ...)	EChk_pass(__VA_ARGS__)
	#define IMGEC_ChkP(act)
	#define TTFEC_P(act, ...)	EChk_pass(__VA_ARGS__)
	#define TTFEC_ChkP(act)
#endif

namespace rs {
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
		static std::string	s_errString;
		static const char* ErrorDesc() {
			const char* err = I::Get();
			if(*err != '\0') {
				s_errString = err;
				I::Reset();
				return s_errString.c_str();
			}
			return nullptr;
		}
		static const char *const GetAPIName() {
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
			Mutex(const Mutex& m) = delete;
			Mutex& operator = (const Mutex& m) = delete;
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
	//! 再帰対応のスピンロック
	template <class T>
	class SpinLock {
		struct Inner {
			SpinLock&	_src;
			T*			_data;

			Inner(const Inner&) = delete;
			Inner& operator = (const Inner&) = delete;
			Inner(Inner&& n): _src(n._src), _data(n._data) {}
			Inner(SpinLock& src, T* data): _src(src), _data(data) {}
			~Inner() {
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
				if(SDL_AtomicCAS(&_atmLock, 0, tls_threadID) == SDL_TRUE)
					bSuccess = true;
				else if(SDL_AtomicGet(&_atmLock) == tls_threadID) {
					// 同じスレッドからのロック
					bSuccess = true;
				}

				if(bSuccess) {
					// ロック成功
					SDL_AtomicAdd(&_atmCount, 1);
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
		boost::optional<Holder> _holder;

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
				ths->_holder->inorder([ths](Args... args){ ths->runIt(args...); });
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
		protected:
			std::exception_ptr	_eptr;
			virtual RET run(Args...) = 0;
		public:
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
				// スレッド実行中だったらエラーを出す
				Assert(Trap, !isRunning())
				SDL_DestroyCond(_condC);
				SDL_DestroyCond(_condP);
			}
			template <class... Args0>
			void start(Args0&&... args0) {
				_holder = boost::in_place(std::forward<Args0>(args0)...);
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
			template <class... Ts>
			static void SetGLAttributes(Ts... ts) {}
			template <class... Ts>
			static void SetGLAttributes(SDL_GLattr attr, int value, Ts... ts) {
				SDL_GL_SetAttribute(attr, value);
				SetGLAttributes(ts...);
			}
			static void SetStdGLAttributes(int major, int minor, int depth);
			static SPWindow Create(const std::string& title, int w, int h, uint32_t flag=0, bool bShare=false);
			static SPWindow Create(const std::string& title, int x, int y, int w, int h, uint32_t flag=0, bool bShare=false);
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

			static int _ReadMode(const char* mode);
			void _clear();
			RWops(SDL_RWops* ops, int access, Type type, EndCB cb, const void* ptr=nullptr, size_t sz=0);
		public:
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
	#define mgr_rw rs::RWMgr::_ref()
	class RWMgr : public spn::ResMgrN<RWops, RWMgr> {
		public:
			using base_type = spn::ResMgrN<RWops, RWMgr>;
			using LHdl = AnotherLHandle<RWops>;
			LHdl fromFile(spn::ToPathStr path, const char* mode, bool bNotKey=false);
			template <class T>
			LHdl fromVector(T&& t) {
				return base_type::acquire(RWops::FromVector(std::forward<T>(t))); }
			LHdl fromConstMem(const void* p, int size, typename RWops::EndCB cb=nullptr);
			LHdl fromMem(void* p, int size, typename RWops::EndCB cb=nullptr);
			LHdl fromFP(FILE* fp, bool bAutoClose, const char* mode);
	};
	DEF_HANDLE(RWMgr, RW, RWops)

	struct Color {
		//! big endian で見た時のピクセルフォーマット
		enum PFormat {
			RGB8, RGBA8, ARGB8, RGB555, RGBA5551, ARGB1555
		};
		//! SDLフォーマットとの対応付け
		struct Info {
			uint32_t sdlformat,
					bitsize,
					maskR, maskG, maskB, maskA;
		};
		const static Info c_pfInfo[];

		static uint32_t Map(PFormat format, int r, int g, int b);
		static uint32_t Map(PFormat format, int r, int g, int b, int a);
		template <class R, class G, class B>
		static uint32_t Get(PFormat format, uint32_t pixel, R& r, G& g, B& b) {
			int tmp;
			Get(format, pixel, r, g, b, tmp);
		}
		template <class R, class G, class B, class A>
		static uint32_t Get(PFormat format, uint32_t pixel, R& r, G& g, B& b, A& a) {
			r = 0;
			g = 0;
			b = 0;
			a = 0;
			SDLEC_P(Trap, SDL_GetRGBA, pixel, c_pfInfo[format].sdlformat,
				reinterpret_cast<uint8_t*>(&r),
				reinterpret_cast<uint8_t*>(&g),
				reinterpret_cast<uint8_t*>(&b),
				reinterpret_cast<uint8_t*>(&a));
		}
	};

	class Surface;
	using SPSurface = std::shared_ptr<Surface>;
	class Surface {
		SDL_Surface*	_sfc;
		Mutex			_mutex;
		class LockObj {
			Surface& 	_sfc;
			void*		_bits;
			int			_pitch;

			public:
				LockObj(LockObj&& lk);
				LockObj(Surface& sfc, void* bits, int pitch);
				~LockObj();
				void* getBits();
				int getPitch();
				operator bool () const;
		};
		void _unlock();

		Surface(SDL_Surface* sfc);
		public:
			//! 任意のフォーマットの画像を読み込む
			static SPSurface Load(HRW hRW);
			//! 空のサーフェス作成
			static SPSurface Create(int w, int h, Color::PFormat format);
			//! ピクセルデータを元にサーフェス作成
			static SPSurface Create(void* pSrc, int pitch, int w, int h, Color::PFormat format);

			~Surface();
			void saveAsBMP(HRW hDst) const;
			void saveAsPNG(HRW hDst) const;
			void fillRect(const spn::Rect& rect, uint32_t color);
			LockObj lock();
			LockObj try_lock();
			spn::Size getSize() const;
			const SDL_PixelFormat& getFormat() const;
			int width() const;
			int height() const;
	};
}
