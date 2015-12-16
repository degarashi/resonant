#include "sys_uniform.hpp"
#include "glx_if.hpp"

namespace rs {
	using GlxId = IEffect::GlxId;
	namespace unif {
		const IdValue Alpha = GlxId::GenUnifId("u_alpha"),
					Color = GlxId::GenUnifId("u_color");
		namespace texture {
			const IdValue Diffuse = GlxId::GenUnifId("u_texDiffuse");
		}
	}
	namespace sysunif {
		namespace screen {
			const IdValue Size = GlxId::GenUnifId("sys_vScreenSize");
		}
	}

	// --------------------- SystemUniform ---------------------
	namespace {
		using SetF = std::function<void (const SystemUniform&, IEffect&)>;
		const SetF c_systagF[] = {
			[](const SystemUniform& s, IEffect& e) {
				auto& ss = s.getScreenSize();
				e.setUniform<false>(sysunif::screen::Size,
									spn::Vec4(ss.width,
										ss.height,
										spn::Rcp22Bit(ss.width),
										spn::Rcp22Bit(ss.height)));
			}
		};
	}
	void SystemUniform::moveFrom(SystemUniform& prev) {
		_screenSize = prev._screenSize;
	}
	const spn::Size& SystemUniform::getScreenSize() const {
		return _screenSize;
	}
	void SystemUniform::setScreenSize(const spn::Size& s) {
		_screenSize = s;
	}
	void SystemUniform::outputUniforms(IEffect& e) const {
		for(auto& f : c_systagF)
			f(*this, e);
	}
}

