#include "gameloop.hpp"

namespace rs {
	AppPath::AppPath(const std::string& apppath): _pbApp(apppath), _pbAppDir(_pbApp) {
		Assert(Trap, _pbApp.isAbsolute())
	}
	void AppPath::setFromText(HRW hRW) {
		RWops& ops = hRW.ref();
		spn::ByteBuff buff = ops.readAll();
		const char *ptr = reinterpret_cast<const char*>(&buff[0]),
					*ptrE = ptr + buff.size();
		int i=0;
		char tmp[256];
		int wcur = 0;
		while(ptr != ptrE) {
			char c = *ptr++;
			if(c == '\n') {
				if(wcur == 0)
					continue;
				tmp[wcur] = '\0';
				_path[i] = _pbAppDir;
				_path[i] <<= tmp;
				++i;
				wcur = 0;
			} else
				tmp[wcur++] = c;
		}
		Assert(Warn, i==static_cast<int>(Type::NumType))
	}
	const spn::PathBlock& AppPath::getPath(Type typ) const {
		return _path[static_cast<int>(typ)];
	}
	const spn::PathBlock& AppPath::getAppPath() const {
		return _pbApp;
	}
	const spn::PathBlock& AppPath::getAppDir() const {
		return _pbAppDir;
	}
}
