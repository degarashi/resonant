#include "systeminfo.hpp"

namespace rs {
	SystemInfo::SystemInfo():
		_scrSize{0,0},
		_fps(0)
	{}
	void SystemInfo::setInfo(const spn::SizeF& sz, int fps) {
		_scrSize = sz;
		_fps = fps;
	}
	const spn::SizeF& SystemInfo::getScreenSize() const {
		return _scrSize;
	}
	int SystemInfo::getFPS() const {
		return _fps;
	}
}
