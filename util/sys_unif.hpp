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
