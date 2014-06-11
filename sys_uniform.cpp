#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	// --------------------- EUnif ---------------------
	const std::string& EUnif::Get(value_t t) {
		return cs_word[t];
	}
	const std::string EUnif::cs_word[] = {
		"texDiffuse"
	};

	// --------------------- SysUnif ---------------------
	const std::string SysUnif::cs_word[] = {
		"sys_screen"
	};
	const std::string& SysUnif::Get(value_t t) {
		return cs_word[t];
	}

	// --------------------- SystemUniformBase ---------------------
	namespace {
		using SetF = std::function<void (const SystemUniformBase&, GLEffect&)>;
		const SetF c_systagF[] = {
			[](const SystemUniformBase& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif::Get(SysUnif::Screen::size))) {
					auto& ss = s.getScreenSize();
					glx.setUniform(*id, spn::Vec4(ss.width,
												ss.height,
												spn::Rcp22Bit(ss.width),
												spn::Rcp22Bit(ss.height)));
				}
			}
		};
	}
	const spn::Size& SystemUniformBase::getScreenSize() const {
		return _screenSize;
	}
	void SystemUniformBase::setScreenSize(const spn::Size& s) {
		_screenSize = s;
	}
	void SystemUniformBase::outputUniforms(GLEffect& glx) const {
		for(auto& f : c_systagF)
			f(*this, glx);
	}
}

