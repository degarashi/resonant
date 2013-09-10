#include <SDL2/SDL.h>
#include <string>
#include "spinner/misc.hpp"

namespace sdlw {
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
	};
}
