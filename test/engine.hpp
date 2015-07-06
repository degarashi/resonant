#pragma once
#include "../sys_uniform.hpp"
#include "../glx.hpp"

namespace myunif {
	namespace light {
		extern const rs::IdValue	Position,		// "m_vLightPos"
									Color,			// "m_vLightColor"
									Dir,			// "m_vLightDir"
									Power;			// "m_fLightPower"
	}
}
class Engine : public rs::SystemUniform,
				public rs::GLEffect
{
	private:
		rs::SystemUniform2D _unif2d;
		rs::SystemUniform3D	_unif3d;
		void _prepareUniforms();
		#define SEQ_UNIF \
			((LightPosition)(spn::Vec3)) \
			((LightColor)(spn::Vec3)) \
			((LightDir)(spn::Vec3)) \
			((LightPower)(float))
		RFLAG_S(Engine, SEQ_UNIF)
	public:
		using rs::GLEffect::GLEffect;
		rs::SystemUniform2D& ref2d();
		rs::SystemUniform3D& ref3d();
		operator rs::SystemUniform2D& ();
		operator rs::SystemUniform3D& ();
		RFLAG_SETMETHOD_S(SEQ_UNIF)
		RFLAG_REFMETHOD_S(SEQ_UNIF)
		RFLAG_GETMETHOD_S(SEQ_UNIF)
		#undef SEQ_UNIF

		void drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem=0) override;
		void draw(GLenum mode, GLint first, GLsizei count) override;
};
