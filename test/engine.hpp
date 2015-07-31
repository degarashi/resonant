#pragma once
#include "../sys_uniform.hpp"
#include "../glx.hpp"
#include "spinner/unituple/operator.hpp"

namespace rs {
	namespace util {
		#define DEF_SYSUNIF(num) \
			private: \
				SystemUniform##num##D _unif##num##d; \
			public: \
				SystemUniform##num##D& ref##num##d(); \
				operator SystemUniform##num##D& ();

		template <class... Ts>
		class GLEffect_Ts : public GLEffect {
			private:
				std::tuple<Ts...>	_ts;
			protected:
				void _prepareUniforms() override {
					spn::TupleForEach([this](auto& t){
						t.outputUniforms(*this);
					}, _ts);
				}
			public:
				using GLEffect::GLEffect;
				template <class T>
				T& ref() { return std::get<T>(_ts); }
				template <class T>
				operator T& () { return ref<T>(); }
		};
		//! GLEffect + SystemUniform2D
		using GLEffect_2D = GLEffect_Ts<SystemUniform, SystemUniform2D>;
		//! GLEffect + SystemUniform3D
		using GLEffect_3D = GLEffect_Ts<SystemUniform, SystemUniform3D>;
		//! GLEffect + SystemUniform(2D & 3D)
		using GLEffect_2D3D = GLEffect_Ts<SystemUniform, SystemUniform2D, SystemUniform3D>;
	}
}
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
