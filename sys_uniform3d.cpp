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

	namespace {
		using SetF3D = std::function<void (const SystemUniform3D&, GLEffect&)>;
		const SetF3D c_systagF3D[] = {
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::Transform,
					s.getTransform(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::TransformInv,
					s.getTransformInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::Proj,
						hCam.cref().getProj(), true);
			}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::ProjInv,
					s.getProjInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::View,
						hCam.cref().getView(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::ViewInv,
						s.getViewInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::ViewProj,
						hCam.cref().getViewProj(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::ViewProjInv,
						hCam.cref().getViewProjInv(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::World,
					s.getWorld(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				glx.setUniform_try(sysunif3d::matrix::WorldInv,
					s.getWorldInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::EyePos,
						hCam.cref().getPose().getOffset());
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					glx.setUniform_try(sysunif3d::matrix::EyeDir,
						hCam.cref().getPose().getRot().getZAxis());
				}
			}
		};
	}
	SystemUniform3D::SystemUniform3D() {}
	void SystemUniform3D::outputUniforms(GLEffect& glx, bool bBase) const {
		if(bBase)
			SystemUniformBase::outputUniforms(glx);
		for(auto& f : c_systagF3D)
			f(*this, glx);
	}
	void SystemUniform3D::setTransform(const spn::AMat44& m) {
		_rflag.set<Transform>(m);
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, ViewInv*) const {
		auto m4 = getCamera().cref().getView().convertA44();
		m4.inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, ProjInv*) const {
		getCamera().cref().getProj().inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, WorldInv*) const {
		_rflag.get<World>(this).inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, Transform*) const {
		const auto& cd = getCamera().cref();
		m = getWorld() * cd.getViewProj();
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, TransformInv*) const {
		getTransform().inversion(m);
		return 0;
	}
}

