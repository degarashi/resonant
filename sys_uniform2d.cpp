#include "sys_uniform.hpp"
#include "glx.hpp"
#include "camera2d.hpp"

namespace rs {
	using GlxId = GLEffect::GlxId;
	namespace unif2d {
		const IdValue	Depth = GlxId::GenUnifId("sys_fDepth"),
						Alpha = GlxId::GenUnifId("sys_fAlpha");
	}
	namespace sysunif2d {
		namespace matrix {
			const IdValue	World =	GlxId::GenUnifId("sys_mWorld2d"),
							WorldInv = GlxId::GenUnifId("sys_mWorld2dInv"),
							View = GlxId::GenUnifId("sys_mView2d"),
							ViewInv = GlxId::GenUnifId("sys_mView2dInv"),
							Proj = GlxId::GenUnifId("sys_mProj2d"),
							ProjInv = GlxId::GenUnifId("sys_mProj2dInv"),
							ViewProj = GlxId::GenUnifId("sys_mViewProj2d"),
							ViewProjInv = GlxId::GenUnifId("sys_mViewProj2dInv"),
							Transform = GlxId::GenUnifId("sys_mTrans2d"),
							TransformInv = GlxId::GenUnifId("sys_mTrans2dInv");
		}
	}
	uint32_t SystemUniform2D::_refresh(spn::Mat33& m, WorldInv*) const {
		getWorld().inversion(m);
		return 0;
	}
	uint32_t SystemUniform2D::_refresh(spn::Mat33& m, Transform*) const {
		auto& hc = getCamera();
		if(hc)
			m = getWorld() * hc->getViewProj();
		else
			m.identity();
		return 0;
	}
	uint32_t SystemUniform2D::_refresh(spn::Mat33& m, TransformInv*) const {
		getTransform().inversion(m);
		return 0;
	}

	SystemUniform2D::SystemUniform2D(): _acCamera(0) {
		setWorld(spn::Mat33(1, spn::Mat33::TagDiagonal));
	}
	void SystemUniform2D::setCamera(HCam2D hC) {
		_rflag.set<Camera>(hC);
		if(hC)
			_acCamera = hC->getAccum();
	}
	void SystemUniform2D::outputUniforms(GLEffect& glx) const {
		#define DEF_SETUNIF(name, func) \
			if(auto idv = glx.getUnifId(sysunif2d::matrix::name)) \
				glx.setUniform(*idv, func##name(), true);
		DEF_SETUNIF(World, get)
		DEF_SETUNIF(WorldInv, get)
		if(auto& hc = getCamera()) {
			auto& cd = hc.cref();
			auto ac = cd.getAccum();
			if(_acCamera != ac) {
				_acCamera = ac;

				DEF_SETUNIF(View, cd.get)
				DEF_SETUNIF(ViewInv, cd.get)
				DEF_SETUNIF(Proj, cd.get)
				DEF_SETUNIF(ViewProj, cd.get)
				DEF_SETUNIF(ViewProjInv, cd.get)
			}
		}
		DEF_SETUNIF(Transform, get)
		DEF_SETUNIF(TransformInv, get)
		#undef DEF_SETUNIF
	}
}

