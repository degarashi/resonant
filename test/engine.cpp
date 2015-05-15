#include "test.hpp"
#include "../font.hpp"
#include "../sound.hpp"
#include "../glresource.hpp"
#include "../camera.hpp"
#include "../input.hpp"

void Engine::draw(GLenum mode, GLint first, GLsizei count) {
	SystemUniform::outputUniforms(*this);
	SystemUniform3D::outputUniforms(*this);
	GLEffect::draw(mode, first, count);
}
void Engine::drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem) {
	SystemUniform::outputUniforms(*this);
	SystemUniform3D::outputUniforms(*this);
	GLEffect::drawIndexed(mode, count, offsetElem);
}
