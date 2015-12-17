#include "sys_uniform.hpp"
#include "camera.hpp"
#include "glx_if.hpp"

namespace rs {
	SystemUniform3D::Getter::counter_t SystemUniform3D::Getter::operator()(const HLCamF& c, Camera*, const SystemUniform3D&) const {
		if(c)
			return c->getAccum();
		return 0;
	}

	using GlxId = IEffect::GlxId;
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
			const IdValue	Specular = GlxId::GenUnifId("u_texSpecular"),
							Normal = GlxId::GenUnifId("u_texNormal"),
							Emissive = GlxId::GenUnifId("u_texEmissive");
		}
	}

	spn::RFlagRet SystemUniform3D::_refresh(typename Transform::value_type& m, Transform*) const {
		auto ret = _rflag.getWithCheck(this, m);
		auto& cam = *std::get<1>(ret.first);
		bool b = ret.second;
		if(b) {
			m = getWorld();
			if(cam)
				m *= cam->getViewProj();
		}
		return {true, 0};
	}
	spn::RFlagRet SystemUniform3D::_refresh(spn::AMat44& m, TransformInv*) const {
		getTransform().inversion(m);
		return {true, 0};
	}
	spn::RFlagRet SystemUniform3D::_refresh(spn::AMat44& m, WorldInv*) const {
		getWorld().inversion(m);
		return {true, 0};
	}

	SystemUniform3D::SystemUniform3D() {
		auto im = spn::AMat44(1, spn::AMat44::TagDiagonal);
		setWorld(im);
		setTransform(im);
	}
	void SystemUniform3D::moveFrom(SystemUniform3D& prev) {
		_rflag = prev._rflag;
	}
	void SystemUniform3D::outputUniforms(IEffect& e) const {
		#define DEF_SETUNIF(name, func) \
			if(auto idv = e.getUnifId(sysunif3d::matrix::name)) \
				e.setUniform(*idv, func##name(), true);
		DEF_SETUNIF(World, get)
		DEF_SETUNIF(WorldInv, get)
		if(auto& hc = getCamera()) {
			auto& cd = hc.cref();
			DEF_SETUNIF(View, cd.get)
			DEF_SETUNIF(Proj, cd.get)
			DEF_SETUNIF(ViewProj, cd.get)
			DEF_SETUNIF(ViewProjInv, cd.get)

			auto& ps = cd.getPose();
			if(auto idv = e.getUnifId(sysunif3d::matrix::EyePos))
				e.setUniform(*idv, ps.getOffset(), true);
			if(auto idv = e.getUnifId(sysunif3d::matrix::EyeDir))
				e.setUniform(*idv, ps.getRot().getZAxis(), true);
		}
		DEF_SETUNIF(Transform, get)
		DEF_SETUNIF(TransformInv, get)
		#undef DEF_SETUNIF
	}
}

