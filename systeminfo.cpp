#include "systeminfo.hpp"
#include "glhead.hpp"

namespace rs {
	SystemInfo::SystemInfo():
		_scrSize{0,0},
		_fps(0)
	{}
	void SystemInfo::setInfo(const spn::SizeF& sz, int fps) {
		_scrSize = sz;
		_fps = fps;
		GL.glViewport(0, 0, sz.width, sz.height);
	}
	const spn::SizeF& SystemInfo::getScreenSize() const {
		return _scrSize;
	}
	int SystemInfo::getFPS() const {
		return _fps;
	}
}
