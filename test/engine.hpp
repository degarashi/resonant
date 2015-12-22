#pragma once
#include "../util/sys_unif.hpp"
#include "../updater.hpp"
#include "gaussblur.hpp"
#include "spinner/structure/wrapper.hpp"

namespace myunif {
	extern const rs::IdValue	U_Position,		// "u_lightPos"
								U_Color,		// "u_lightColor"
								U_Dir,			// "u_lightDir"
								U_Mat,			// "u_lightMat"
								U_Coeff,		// "u_lightCoeff"
								U_Depth,		// "u_texLightDepth"
								U_CubeDepth,	// "u_texCubeDepth"
								U_LightIVP,		// "u_lightViewProj"
								U_TexZPass,		// "u_texZPass"
								U_TexLAccum,	// "u_texLAccum"
								U_DepthRange,	// "u_depthRange"
								U_LineLength,	// "u_lineLength"
								U_ScrLightPos,	// "u_scrLightPos"
								U_ScrLightDir,	// "u_scrLightDir"
								U_ScreenSize;	// "u_scrSize"
}
extern const rs::IdValue T_PostEffect,
						T_ZPass,
						T_LAccum,
						T_Shading,
						T_LAccumS;
class Engine : public rs::util::GLEffect_2D3D {
	public:
		struct DrawType {
			enum E {
				Normal,
				Depth,
				CubeNormal,
				CubeDepth,
				DL_ZPrePass,
				DL_Depth,
				DL_Shade,
				_Num
			};
		};
	private:
		struct ScreenSize;
		struct Getter : spn::RFlag_Getter<spn::SizeF> {
			using RFlag_Getter::operator ();
			counter_t operator()(const spn::SizeF&, ScreenSize*, const Engine&) const;
		};
		struct D_Camera;
		struct GetterC : spn::RFlag_Getter<uint32_t> {
			using RFlag_Getter::operator ();
			counter_t operator()(const spn::none_t&, D_Camera*, const Engine&) const;
		};
		struct ScLit {
			spn::Vec3	pos,
						dir;
			spn::Vec2	size;
		};
		using ScSize_t = spn::AcCheck<rs::HLTex, Getter>;
		using ScAsp_t = spn::AcCheck<spn::Wrapper<float>, Getter>;
		using ScLit_t = spn::AcCheck<ScLit, GetterC>;
		using IVP_t = spn::AcCheck<spn::Mat44, GetterC>;
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
			((CubeColorBuff)(rs::HLTex)(LightDepthSize)) \
			((ScreenSize)(spn::SizeF)) \
			((D_Camera)(spn::none_t)) \
			((ZPrePassBuff)(ScSize_t)(ScreenSize)) \
			((LightAccumBuff)(ScSize_t)(ScreenSize)) \
			((ScreenAspect)(ScAsp_t)(ScreenSize)) \
			((LightScLit)(ScLit_t)(LightPosition)(LightDir)(D_Camera)) \
			((LightCoeff)(spn::Vec2)) \
			((LightIVP)(IVP_t)(D_Camera)(LightMatrix))
		RFLAG_S(Engine, SEQ_UNIF)

		mutable GaussBlur	_gauss;
		rs::HLFb		_hlFb;
		DrawType::E	_drawType;
		rs::HLDGroup	_hlDg;
		class DrawScene;
		class CubeScene;
		class DLScene;
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
		rs::HLDObj getDLScene(rs::Priority dprio) const;
		void addSceneObject(rs::HDObj hdObj);
		void remSceneObject(rs::HDObj hdObj);
		void clearScene();
		void setDispersion(float d);
		void setOutputFramebuffer(rs::HFb hFb);
		void moveFrom(rs::IEffect& e) override;
};
DEF_LUAIMPORT(Engine)
