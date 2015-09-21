#pragma once
#include "spinner/singleton.hpp"
#include "spinner/size.hpp"

namespace rs {
	class SystemInfo : public spn::Singleton<SystemInfo> {
		private:
			spn::SizeF	_scrSize;
			int			_fps;
		public:
			SystemInfo();
			void setInfo(const spn::SizeF& sz, int fps);
			const spn::SizeF& getScreenSize() const;
			int getFPS() const;
	};
}
#define mgr_info (::rs::SystemInfo::_ref())

#include "luaimport.hpp"
DEF_LUAIMPORT(rs::SystemInfo)
