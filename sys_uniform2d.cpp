#include "sys_uniform.hpp"
#include "glx_if.hpp"
#include "camera2d.hpp"

namespace rs {
	using GlxId = IEffect::GlxId;
	namespace unif2d {
		const IdValue	Depth = GlxId::GenUnifId("sys_fDepth");
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
	spn::RFlagRet SystemUniform2D::_refresh(spn::Mat33& m, WorldInv*) const {
		getWorld().inversion(m);
		return {};
	}
	spn::RFlagRet SystemUniform2D::_refresh(uint32_t& ac, CameraAc*) const {
		getCamera();
		ac = ~0;
		return {};
	}
	spn::RFlagRet SystemUniform2D::_refresh(spn::AcWrapper<spn::Mat33>& m, Transform*) const {
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
	spn::RFlagRet SystemUniform2D::_refresh(spn::Mat33& m, TransformInv*) const {
		getTransform().inversion(m);
		return {};
	}

	SystemUniform2D::SystemUniform2D() {
		setWorld(spn::Mat33(1, spn::Mat33::TagDiagonal));
	}
	void SystemUniform2D::outputUniforms(IEffect& e) const {
		#define DEF_SETUNIF(name, func) \
			if(auto idv = e.getUnifId(sysunif2d::matrix::name)) \
				e.setUniform(*idv, func##name(), true);
		DEF_SETUNIF(World, get)
		DEF_SETUNIF(WorldInv, get)
		if(auto& hc = getCamera()) {
			auto& cd = hc.cref();
			DEF_SETUNIF(View, cd.get)
			DEF_SETUNIF(ViewInv, cd.get)
			DEF_SETUNIF(Proj, cd.get)
			DEF_SETUNIF(ViewProj, cd.get)
			DEF_SETUNIF(ViewProjInv, cd.get)
		}
		DEF_SETUNIF(Transform, get)
		DEF_SETUNIF(TransformInv, get)
		#undef DEF_SETUNIF
	}
}
