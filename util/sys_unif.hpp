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
				void moveFrom(IEffect& prev) override {
					auto& prevE = static_cast<GLEffect_Ts&>(prev);
					spn::TupleForEach([this,&prevE](auto& t){
						using T = std::decay_t<decltype(t)>;
						t.moveFrom(prevE.template ref<T>());
					}, _ts);
				}
		};
		//! GLEffect + SystemUniform2D
		class GLEffect_2D : public GLEffect_Ts<SystemUniform, SystemUniform2D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform2D>::GLEffect_Ts;
				SystemUniform2D& ref2D() override { return ref<SystemUniform2D>(); }
				using GLEffect_Ts::ref2D;
		};
		//! GLEffect + SystemUniform3D
		class GLEffect_3D : public GLEffect_Ts<SystemUniform, SystemUniform3D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform3D>::GLEffect_Ts;
				SystemUniform3D& ref3D() override { return ref<SystemUniform3D>(); }
				using GLEffect_Ts::ref3D;
		};
		//! GLEffect + SystemUniform(2D & 3D)
		class GLEffect_2D3D : public GLEffect_Ts<SystemUniform, SystemUniform2D, SystemUniform3D> {
			public:
				using GLEffect_Ts<SystemUniform, SystemUniform2D, SystemUniform3D>::GLEffect_Ts;
				SystemUniform2D& ref2D() override { return ref<SystemUniform2D>(); }
				SystemUniform3D& ref3D() override { return ref<SystemUniform3D>(); }
				using GLEffect_Ts::ref2D;
				using GLEffect_Ts::ref3D;
		};
	}
}
