#include "sys_uniform.hpp"
#include "camera.hpp"
#include "glx.hpp"

namespace rs {
	const std::string EUnif3D::cs_word[] = {
		"texSpecular",
		"texNormal"
	};
	const std::string& EUnif3D::Get(value_t t) {
		return cs_word[t];
	}
	const std::string SysUnif3D::cs_word[] = {
		"sys_mTrans",
		"sys_mTransInv",
		"sys_mProj",
		"sys_mProjInv",
		"sys_mView",
		"sys_mViewInv",
		"sys_mViewProj",
		"sys_mViewProjInv",
		"sys_mWorld",
		"sys_mWorldInv",
		"sys_vEyePos",
		"sys_vEyeDir"
	};
	const std::string& SysUnif3D::Get(value_t t) {
		return cs_word[t];
	}

	namespace {
		using SetF3D = std::function<void (const SystemUniform3D&, GLEffect&)>;
		const SetF3D c_systagF3D[] = {
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::Transform)))
					glx.setUniform(*id, s.getTransform(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::TransformInv)))
					glx.setUniform(*id, s.getTransformInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::Proj)))
						glx.setUniform(*id, hCam.cref().getProjMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::ProjInv)))
					glx.setUniform(*id, s.getProjInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::View)))
						glx.setUniform(*id, hCam.cref().getViewMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::ViewInv)))
					glx.setUniform(*id, s.getViewInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::ViewProj)))
						glx.setUniform(*id, hCam.cref().getViewProjMatrix(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::ViewProjInv)))
						glx.setUniform(*id, hCam.cref().getViewProjInv(), true);
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::World)))
					glx.setUniform(*id, s.getWorld(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::WorldInv)))
					glx.setUniform(*id, s.getWorldInv(), true);
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::EyePos)))
						glx.setUniform(*id, hCam.cref().getOffset());
				}
			},
			[](const SystemUniform3D& s, GLEffect& glx) {
				if(HCam hCam = s.getCamera()) {
					if(auto id = glx.getUniformID(SysUnif3D::Get(SysUnif3D::Matrix::EyeDir)))
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

