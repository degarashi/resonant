#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	namespace unif {
		namespace texture {
			const std::string Diffuse("texDiffuse");
		}
	}
	namespace sysunif {
		namespace screen {
			const std::string Size("sys_screen");
		}
	}

	// --------------------- SystemUniformBase ---------------------
	namespace {
		using SetF = std::function<void (const SystemUniformBase&, GLEffect&)>;
		const SetF c_systagF[] = {
			[](const SystemUniformBase& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif::screen::Size)) {
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

