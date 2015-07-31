#include "engine.hpp"

namespace myunif {
	namespace light {
		using GlxId = rs::GLEffect::GlxId;
		const rs::IdValue Position = GlxId::GenUnifId("m_vLightPos"),
							Color = GlxId::GenUnifId("m_vLightColor"),
							Dir =	GlxId::GenUnifId("m_vLightDir"),
							Power =	GlxId::GenUnifId("m_fLightPower");
	}
}
void Engine::_prepareUniforms() {
	rs::util::GLEffect_2D3D::_prepareUniforms();

	#define DEF_SETUNIF(name, func) \
		if(auto idv = getUnifId(myunif::light::name)) \
			setUniform(*idv, func##name(), true);
	DEF_SETUNIF(Position, getLight)
	DEF_SETUNIF(Color, getLight)
	DEF_SETUNIF(Dir, getLight)
	DEF_SETUNIF(Power, getLight)
	#undef DEF_SETUNIF
}
