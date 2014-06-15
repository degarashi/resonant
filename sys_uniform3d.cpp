#include "sys_uniform.hpp"
#include "camera.hpp"
#include "glx.hpp"

namespace rs {
	namespace sysunif3d {
		namespace matrix {
			const std::string Transform("sys_mTrans"),
							TransformInv("sys_mTransInv"),
							Proj("sys_mProj"),
							ProjInv("sys_mProjInv"),
							View("sys_mView"),
							ViewInv("sys_mViewInv"),
							ViewProj("sys_mViewProj"),
							ViewProjInv("sys_mViewProjInv"),
							World("sys_mWorld"),
							WorldInv("sys_mWorldInv"),
							EyePos("sys_vEyePos"),
							EyeDir("sys_vEyeDir");
		}
	}
	namespace unif3d {
		namespace texture {
			const std::string Specular("texSpecular"),
								Normal("texNormal");
		}
	}

	namespace {
		using SetF3D = std::function<void (const SystemUniform3D&, GLEffect&)>;
		const SetF3D c_systagF3D[] = {
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::Transform))
					glx.setUniform(*id, s.getTransform(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::TransformInv))
					glx.setUniform(*id, s.getTransformInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::Proj))
						glx.setUniform(*id, hCam.cref().getProjMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::ProjInv))
					glx.setUniform(*id, s.getProjInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::View))
						glx.setUniform(*id, hCam.cref().getViewMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::ViewInv))
					glx.setUniform(*id, s.getViewInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::ViewProj))
						glx.setUniform(*id, hCam.cref().getViewProjMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::ViewProjInv))
						glx.setUniform(*id, hCam.cref().getViewProjInv(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::World))
					glx.setUniform(*id, s.getWorld(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(sysunif3d::matrix::WorldInv))
					glx.setUniform(*id, s.getWorldInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::EyePos))
						glx.setUniform(*id, hCam.cref().getOffset());
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(sysunif3d::matrix::EyeDir))
						glx.setUniform(*id, hCam.cref().getRot().getZAxis());
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
		auto m4 = getCamera().cref().getViewMatrix().convertA44();
		m4.inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, ProjInv*) const {
		getCamera().cref().getProjMatrix().inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, WorldInv*) const {
		_rflag.get<World>(this).inversion(m);
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, Transform*) const {
		const CamData& cd = getCamera().cref();
		m = getWorld() * cd.getViewProjMatrix();
		return 0;
	}
	uint32_t SystemUniform3D::_refresh(spn::AMat44& m, TransformInv*) const {
		getTransform().inversion(m);
		return 0;
	}
}

