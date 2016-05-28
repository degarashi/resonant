#pragma once
#include "../util/sys_unif.hpp"
#include "../updater.hpp"
#include "gaussblur.hpp"
#include "reduction.hpp"
#include "dlight.hpp"
#include "offsetadd.hpp"

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
		using ScSize_t = spn::AcCheck<rs::HLTex, Getter>;
		struct ScInfo_t {
			float		aspect;
			spn::Vec2	size;
		};
		using ScInfo_a = spn::AcCheck<ScInfo_t, GetterC>;
		#define SEQ_UNIF \
			((LineLength)(float)) \
			((LightDepthSize)(spn::Size)) \
			((LightDepth)(rs::HLRb)(LightDepthSize)) \
			((LightColorBuff)(rs::HLTex)(LightDepthSize)) \
			((LightFB)(rs::HLFb)(LightDepth)(LightColorBuff)) \
			((CubeColorBuff)(rs::HLTex)(LightDepthSize)) \
			((ScreenSize)(spn::SizeF)) \
			((ZPrePassBuff)(ScSize_t)(ScreenSize)) \
			((LightAccumBuff)(ScSize_t)(ScreenSize)) \
			((D_Camera)(spn::none_t)) \
			((ScreenInfo)(ScInfo_a)(ScreenSize)(D_Camera))
		RFLAG_S(Engine, SEQ_UNIF)

		using DLightNS = spn::noseq_list<DLight>;
		using LitId = DLightNS::id_type;
		DLightNS			_light;
		using LitOP = spn::Optional<const DLight&>;
		LitOP				_activeLight;
		GaussBlur			_gauss;
		Reduction			_reduction;
		OffsetAdd			_ofs;

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
		void setOffset(float ofs);
		rs::HLFb setOutputFramebuffer(rs::VHFb hFb);
		void moveFrom(rs::IEffect& e) override;

		LitId makeLight();
		void remLight(LitId id);
		DLight& getLight(LitId id);
};
DEF_LUAIMPORT(Engine)
