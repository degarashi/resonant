#include "sys_uniform.hpp"
#include "glx_if.hpp"
#include "camera2d.hpp"

namespace rs {
	SystemUniform2D::Getter::counter_t SystemUniform2D::Getter::operator()(const HLCam2DF& c, Camera*) const {
		if(c)
			return c->getAccum();
		return 0;
	}

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
		return {true, 0};
	}
	spn::RFlagRet SystemUniform2D::_refresh(typename Transform::value_type& m, Transform*) const {
		auto ret = _rflag.getWithCheck(this, m);
		auto& cam = *std::get<1>(ret.first);
		bool b = ret.second;
		if(b) {
			m = getWorld();
			if(cam)
				m *= cam->getViewProj();
		}
		return {b, 0};
	}
	spn::RFlagRet SystemUniform2D::_refresh(spn::Mat33& m, TransformInv*) const {
		getTransform().inversion(m);
		return {true, 0};
	}

	SystemUniform2D::SystemUniform2D() {
		auto im = spn::Mat33(1, spn::Mat33::TagDiagonal);
		setWorld(im);
		setTransform(im);
	}
	void SystemUniform2D::moveFrom(SystemUniform2D& prev) {
		_rflag = prev._rflag;
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
