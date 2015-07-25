#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	using GlxId = GLEffect::GlxId;
	namespace unif {
		const IdValue Alpha = GlxId::GenUnifId("sys_fAlpha"),
					Color = GlxId::GenUnifId("sys_vColor");
		namespace texture {
			const IdValue Diffuse = GlxId::GenUnifId("sys_texDiffuse");
		}
	}
	namespace sysunif {
		namespace screen {
			const IdValue Size = GlxId::GenUnifId("sys_vScreenSize");
		}
	}

	// --------------------- SystemUniform ---------------------
	namespace {
		using SetF = std::function<void (const SystemUniform&, GLEffect&)>;
		const SetF c_systagF[] = {
			[](const SystemUniform& s, GLEffect& glx) {
				auto& ss = s.getScreenSize();
				glx.setUniform_try(sysunif::screen::Size,
									spn::Vec4(ss.width,
										ss.height,
										spn::Rcp22Bit(ss.width),
										spn::Rcp22Bit(ss.height)));
			}
		};
	}
	const spn::Size& SystemUniform::getScreenSize() const {
		return _screenSize;
	}
	void SystemUniform::setScreenSize(const spn::Size& s) {
		_screenSize = s;
	}
	void SystemUniform::outputUniforms(GLEffect& glx) const {
		for(auto& f : c_systagF)
			f(*this, glx);
	}
}

