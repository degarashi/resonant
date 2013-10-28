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
	Spec::PStat Spec::powerStatus() const {
		PStat ps;
		switch(SDL_GetPowerInfo(&ps.seconds, &ps.percentage)) {
			case SDL_POWERSTATE_ON_BATTERY:
				ps.state = PStatN::OnBattery; break;
			case SDL_POWERSTATE_NO_BATTERY:
				ps.state = PStatN::NoBattery; break;
			case SDL_POWERSTATE_CHARGING:
				ps.state = PStatN::Charging; break;
			case SDL_POWERSTATE_CHARGED:
				ps.state = PStatN::Charged; break;
			default:
				ps.state = PStatN::Unknown;
		}
		return ps;
	}

	void Spec::PStat::output(std::ostream& os) const {
		if(state == PStatN::Unknown)
			os << "Unknown state";
		else if(state == PStatN::NoBattery)
			os << "No battery";
		else {
			os << "Battery state:" << std::endl;
			if(seconds == -1) os << "unknown time left";
			else os << seconds << " seconds left";
			os << std::endl;

			if(percentage == -1) os << "unknown percentage left";
			else os << percentage << " percent left";
		}
		os << std::endl;
	}
}
