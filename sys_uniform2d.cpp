#include "sys_uniform.hpp"
#include "glx.hpp"

namespace rs {
	using GlxId = GLEffect::GlxId;
	namespace unif2d {
		const IdValue	Depth = GlxId::GenUnifId("sys_fDepth"),
						Alpha = GlxId::GenUnifId("sys_fAlpha");
	}
	namespace sysunif2d {
		namespace matrix {
			const IdValue Transform = GlxId::GenUnifId("sys_mTrans2D");
		}
	}
	namespace {
		using SetF = std::function<void (const SystemUniform2D&, GLEffect&)>;
		const SetF c_systagF2D[] = {
			[](const SystemUniform2D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif2d::matrix::Transform,
								s.getTransform2D(), true);
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
		m *= spn::Mat22::Rotation(getViewRotation()).convert33();
		m *= spn::Mat33::Translation(getViewOffset());
		m.convert(dst);
		return 0;
	}
}

