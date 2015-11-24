#pragma once
#include "../util/sys_unif.hpp"

namespace myunif {
	namespace light {
		extern const rs::IdValue	Position,		// "m_vLightPos"
									Color,			// "m_vLightColor"
									Dir,			// "m_vLightDir"
									Power;			// "m_fLightPower"
	}
}
class Engine : public rs::util::GLEffect_2D3D {
	private:
		#define SEQ_UNIF \
			((LightPosition)(spn::Vec3)) \
			((LightColor)(spn::Vec3)) \
			((LightDir)(spn::Vec3)) \
			((LightPower)(float))
		RFLAG_S(Engine, SEQ_UNIF)
	protected:
		void _prepareUniforms() override;
	public:
		using rs::util::GLEffect_2D3D::GLEffect_2D3D;
		RFLAG_SETMETHOD_S(SEQ_UNIF)
		RFLAG_REFMETHOD_S(SEQ_UNIF)
		RFLAG_GETMETHOD_S(SEQ_UNIF)
		#undef SEQ_UNIF
};
DEF_LUAIMPORT(Engine)
