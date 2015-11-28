#pragma once
#include "../util/sys_unif.hpp"
#include "../updater.hpp"

namespace myunif {
	namespace light {
		extern const rs::IdValue	Position,		// "m_vLightPos"
									Color,			// "m_vLightColor"
									Dir,			// "m_vLightDir"
									Power,			// "m_fLightPower"
									Depth,			// "m_texLightDepth"
									DepthRange,		// "m_depthRange"
									LightMat;		// "m_mLight"
	}
}
extern const rs::IdValue T_PostEffect;
class Engine : public rs::util::GLEffect_2D3D {
	public:
		struct DrawType {
			enum E {
				Normal,
				Depth,
				_Num
			};
		};
	private:
		#define SEQ_UNIF \
			((LightColor)(spn::Vec3)) \
			((LightPower)(float)) \
			((DepthRange)(spn::Vec2)) \
			((LightPosition)(spn::Vec3)) \
			((LightDir)(spn::Vec3)) \
			((LightCamera)(rs::HLCam)(LightPosition)(LightDir)) \
			((LightMatrix)(spn::Mat44)(LightPosition)(LightDir)) \
			((LightDepthSize)(spn::Size)) \
			((LightDepth)(rs::HLTex)(LightDepthSize)) \
			((LightColorBuff)(rs::HLTex)(LightDepthSize)) \
			((LightFB)(rs::HLFb)(LightDepth)(LightColorBuff))
		RFLAG_S(Engine, SEQ_UNIF)

		DrawType::E	_drawType;
		rs::HLDGroup	_hlDg;
		class DrawScene;
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
		void addSceneObject(rs::HDObj hdObj);
		void remSceneObject(rs::HDObj hdObj);
};
DEF_LUAIMPORT(Engine)
