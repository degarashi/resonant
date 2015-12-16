#pragma once
#include "../util/sys_unif.hpp"
#include "../updater.hpp"
#include "gaussblur.hpp"

namespace myunif {
	extern const rs::IdValue	U_Position,		// "u_lightPos"
								U_Color,		// "u_lightColor"
								U_Dir,			// "u_lightDir"
								U_Mat,			// "u_lightMat"
								U_Depth,		// "u_texLightDepth"
								U_CubeDepth,	// "u_texCubeDepth"
								U_DepthRange,	// "u_depthRange"
								U_LineLength;	// "u_lineLength"
}
extern const rs::IdValue T_PostEffect;
class Engine : public rs::util::GLEffect_2D3D {
	public:
		struct DrawType {
			enum E {
				Normal,
				Depth,
				CubeNormal,
				CubeDepth,
				_Num
			};
		};
	private:
		#define SEQ_UNIF \
			((LineLength)(float)) \
			((LightColor)(spn::Vec3)) \
			((DepthRange)(spn::Vec2)) \
			((LightPosition)(spn::Vec3)) \
			((LightDir)(spn::Vec3)) \
			((LightCamera)(rs::HLCam)(LightPosition)(LightDir)) \
			((LightMatrix)(spn::Mat44)(LightPosition)(LightDir)) \
			((LightDepthSize)(spn::Size)) \
			((LightDepth)(rs::HLRb)(LightDepthSize)) \
			((LightColorBuff)(rs::HLTex)(LightDepthSize)) \
			((LightFB)(rs::HLFb)(LightDepth)(LightColorBuff)) \
			((CubeColorBuff)(rs::HLTex)(LightDepthSize))
		RFLAG_S(Engine, SEQ_UNIF)

		mutable GaussBlur	_gauss;
		rs::HLFb		_hlFb;
		DrawType::E	_drawType;
		rs::HLDGroup	_hlDg;
		class DrawScene;
		class CubeScene;
	protected:
		void _prepareUniforms() override;
	public:
		Engine(const std::string& name);
		~Engine();
		DrawType::E getDrawType() const;

		RFLAG_SETMETHOD_S(SEQ_UNIF)
		RFLAG_REFMETHOD_S(SEQ_UNIF)
		RFLAG_GETMETHOD_S(SEQ_UNIF)
		#undef SEQ_UNIF

		rs::HLDObj getDrawScene(rs::Priority dprio) const;
		rs::HLDObj getCubeScene(rs::Priority dprio) const;
		void addSceneObject(rs::HDObj hdObj);
		void remSceneObject(rs::HDObj hdObj);
		void clearScene();
		void setDispersion(float d);
		void setOutputFramebuffer(rs::HFb hFb);
		void moveFrom(rs::IEffect& e) override;
};
DEF_LUAIMPORT(Engine)
