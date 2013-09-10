#include "sdlwrap.hpp"

namespace sdlw {
	namespace {
		using FeatFunc = SDL_bool (*)();
		const FeatFunc cs_ff[] = {
			SDL_Has3DNow,
			SDL_HasAltiVec,
			SDL_HasMMX,
			SDL_HasRDTSC,
			SDL_HasSSE,
			SDL_HasSSE2,
			SDL_HasSSE3,
			SDL_HasSSE41,
			SDL_HasSSE42,
			nullptr
		};
	}

	Spec::Spec(): _platform(SDL_GetPlatform()) {
		_nCacheLine = SDL_GetCPUCacheLineSize();
		_nCpu = SDL_GetCPUCount();

		uint32_t feat = 0;
		auto* f = cs_ff;
		uint32_t bit = 0x01;
		while(*f) {
			if((*f)())
				feat |= bit;
			bit <<= 1;
			++f;
		}
		_feature = feat;
	}
	const std::string& Spec::getPlatform() const {
		return _platform;
	}
	int Spec::cpuCacheLineSize() const {
		return _nCacheLine;
	}
	int Spec::cpuCount() const {
		return _nCpu;
	}
	bool Spec::hasFuture(uint32_t flag) const {
		return _feature & flag;
	}
}
