#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	using GlxId = GLEffect::GlxId;
	namespace unif {
		namespace texture {
			const IdValue Diffuse = GlxId::GenUnifId("sys_texDiffuse");
		}
	}
	namespace sysunif {
		namespace screen {
			const IdValue Size = GlxId::GenUnifId("sys_vScreenSize");
		}
	}

	// --------------------- SystemUniformBase ---------------------
	namespace {
		using SetF = std::function<void (const SystemUniformBase&, GLEffect&)>;
		const SetF c_systagF[] = {
			[](const SystemUniformBase& s, GLEffect& glx) {
				auto& ss = s.getScreenSize();
				glx.setUniform_try(sysunif::screen::Size,
									spn::Vec4(ss.width,
										ss.height,
										spn::Rcp22Bit(ss.width),
										spn::Rcp22Bit(ss.height)));
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

