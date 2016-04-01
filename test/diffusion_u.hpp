#pragma once
#include "../glx_if.hpp"
#include "spinner/vector.hpp"

extern const rs::IdValue
				U_SdScale,
				U_SdDivide,
				U_Rayleigh,
				U_Mie,
				U_LPower,
				U_LDir,
				U_LColor;

class RayleighMie {
	private:
		spn::Vec3	_lDir,
					_lColor,
					_rayleigh;
		float		_sdDivide,
					_mie,
					_mieGain,
					_lPower;
	public:
		RayleighMie();
		void setDivide(float d);
		void setRayleighCoeff(const spn::Vec3& r);
		void setMieCoeff(float gain, float c);
		void setLightDir(const spn::Vec3& d);
		void setLightColor(const spn::Vec3& c);
		void setLightPower(float p);
		void outputParams(rs::IEffect& e) const;
};
