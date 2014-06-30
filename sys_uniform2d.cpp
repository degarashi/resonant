#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	namespace unif2d {
		const std::string	Depth("fDepth"),
							Alpha("fAlpha");
	}
	namespace sysunif2d {
		namespace matrix {
			const std::string	Transform("sys_mTrans2D");
		}
	}
	namespace {
		using SetF = std::function<void (const SystemUniform2D&, GLEffect&)>;
		const SetF c_systagF2D[] = {
			[](const SystemUniform2D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif2d::matrix::Transform))
					glx.setUniform(*id, s.getTransform2D(), true);
			}
		};
	}
	void SystemUniform2D::setTransform2D(const spn::Mat32& m) {
		_rflag.set<Transform2D>(m);
	}
	void SystemUniform2D::outputUniforms(GLEffect& glx, bool bBase) const {
		if(bBase)
			SystemUniformBase::outputUniforms(glx);
		for(auto& f : c_systagF2D)
			f(*this, glx);
	}
	uint32_t SystemUniform2D::_refresh(spn::Mat32& dst, Transform2D*) const {
		const auto& s = getViewScale();
		auto m = spn::Mat33::Scaling(s.x, s.y, 1);
		m *= spn::Mat22::Rotation(spn::DEGtoRAD(getViewRotation())).convert33();
		m *= spn::Mat33::Translation(getViewOffset());
		m.convert(dst);
		return 0;
	}
}

