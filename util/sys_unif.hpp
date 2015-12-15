#pragma once
#include "../sys_uniform.hpp"
#include "../glx.hpp"
#include "spinner/unituple/operator.hpp"

namespace rs {
	namespace util {
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
		class GLEffect_2D : public GLEffect_Ts<SystemUniform, SystemUniform2D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform2D>::GLEffect_Ts;
				SystemUniform2D& ref2D() override { return ref<SystemUniform2D>(); }
		};
		//! GLEffect + SystemUniform3D
		class GLEffect_3D : public GLEffect_Ts<SystemUniform, SystemUniform3D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform3D>::GLEffect_Ts;
				SystemUniform3D& ref3D() override { return ref<SystemUniform3D>(); }
		};
		//! GLEffect + SystemUniform(2D & 3D)
		class GLEffect_2D3D : public GLEffect_Ts<SystemUniform, SystemUniform2D, SystemUniform3D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform2D, SystemUniform3D>::GLEffect_Ts;
				SystemUniform2D& ref2D() override { return ref<SystemUniform2D>(); }
				SystemUniform3D& ref3D() override { return ref<SystemUniform3D>(); }
		};
	}
}
