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
	SystemUniform::outputUniforms(*this);
	_unif2d.outputUniforms(*this);
	_unif3d.outputUniforms(*this);

	#define DEF_SETUNIF(name, func) \
		if(auto idv = getUnifId(myunif::light::name)) \
			setUniform(*idv, func##name(), true);
	DEF_SETUNIF(Position, getLight)
	DEF_SETUNIF(Color, getLight)
	DEF_SETUNIF(Dir, getLight)
	DEF_SETUNIF(Power, getLight)
	#undef DEF_SETUNIF
}
rs::SystemUniform2D& Engine::ref2d() {
	return _unif2d;
}
rs::SystemUniform3D& Engine::ref3d() {
	return _unif3d;
}
Engine::operator rs::SystemUniform2D& (){
	return _unif2d;
}
Engine::operator rs::SystemUniform3D& (){
	return _unif3d;
}
void Engine::draw(GLenum mode, GLint first, GLsizei count) {
	_prepareUniforms();
	GLEffect::draw(mode, first, count);
}
void Engine::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
	_prepareUniforms();
	GLEffect::drawIndexed(mode, count, offsetElem);
}
