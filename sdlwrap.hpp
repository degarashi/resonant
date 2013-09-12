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
}
