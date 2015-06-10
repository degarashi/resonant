#include "sys_uniform.hpp"
#include "camera.hpp"
#include "glx.hpp"

namespace rs {
	using GlxId = GLEffect::GlxId;
	namespace sysunif3d {
		namespace matrix {
			const IdValue	Transform = GlxId::GenUnifId("sys_mTrans"),
							TransformInv = GlxId::GenUnifId("sys_mTransInv"),
							Proj = GlxId::GenUnifId("sys_mProj"),
							ProjInv = GlxId::GenUnifId("sys_mProjInv"),
							View = GlxId::GenUnifId("sys_mView"),
							ViewInv = GlxId::GenUnifId("sys_mViewInv"),
							ViewProj = GlxId::GenUnifId("sys_mViewProj"),
							ViewProjInv = GlxId::GenUnifId("sys_mViewProjInv"),
							World = GlxId::GenUnifId("sys_mWorld"),
							WorldInv = GlxId::GenUnifId("sys_mWorldInv"),
							EyePos = GlxId::GenUnifId("sys_vEyePos"),
							EyeDir = GlxId::GenUnifId("sys_vEyeDir");
		}
	}
	namespace unif3d {
		namespace texture {
			const IdValue	Specular = GlxId::GenUnifId("texSpecular"),
							Normal = GlxId::GenUnifId("texNormal"),
							Emissive = GlxId::GenUnifId("texEmissive");
		}
	}

	spn::RFlagRet SystemUniform3D::_refresh(uint32_t& ac, CameraAc*) const {
		getCamera();
		ac = ~0;
		return {};
	}
	spn::RFlagRet SystemUniform3D::_refresh(spn::AcWrapper<spn::AMat44>& m, Transform*) const {
		auto& hc = getCamera();
		auto cur_ac = getCameraAc();
		if(hc) {
			bool b = spn::CompareAndSet(m.ac_counter, _rflag.getAcCounter<World>());
			auto n_ac = hc->getAccum();
			b |= cur_ac != n_ac;
			if(b) {
				_rflag.ref<CameraAc>() = n_ac;
				m = getWorld() * hc->getViewProj();
			}
			// 次回も更新チェックする
			return {0, true};
		} else
			m = getWorld();
		return {};
	}
	spn::RFlagRet SystemUniform3D::_refresh(spn::AMat44& m, TransformInv*) const {
		getTransform().inversion(m);
		return {};
	}
	spn::RFlagRet SystemUniform3D::_refresh(spn::AMat44& m, WorldInv*) const {
		getWorld().inversion(m);
		return {};
	}

	SystemUniform3D::SystemUniform3D() {
		setWorld(spn::AMat44(1, spn::AMat44::TagDiagonal));
	}
	void SystemUniform3D::outputUniforms(GLEffect& glx) const {
		#define DEF_SETUNIF(name, func) \
			if(auto idv = glx.getUnifId(sysunif3d::matrix::name)) \
				glx.setUniform(*idv, func##name(), true);
		DEF_SETUNIF(World, get)
		DEF_SETUNIF(WorldInv, get)
		if(auto& hc = getCamera()) {
			auto& cd = hc.cref();
			DEF_SETUNIF(View, cd.get)
			DEF_SETUNIF(Proj, cd.get)
			DEF_SETUNIF(ViewProj, cd.get)
			DEF_SETUNIF(ViewProjInv, cd.get)

			auto& ps = cd.getPose();
			if(auto idv = glx.getUnifId(sysunif3d::matrix::EyePos))
				glx.setUniform(*idv, ps.getOffset(), true);
			if(auto idv = glx.getUnifId(sysunif3d::matrix::EyeDir))
				glx.setUniform(*idv, ps.getRot().getZAxis(), true);
		}
		DEF_SETUNIF(Transform, get)
		DEF_SETUNIF(TransformInv, get)
		#undef DEF_SETUNIF
	}
}

