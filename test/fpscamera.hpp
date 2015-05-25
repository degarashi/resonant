#pragma once
#include "../handle.hpp"

class FPSCamera : public rs::ObjectT<FPSCamera, 0x0000> {
	private:
		bool		_bPress;
		spn::DegF	_yaw, _pitch, _roll;
		struct St_Default;
		void initState() override;
	public:
		FPSCamera();
};
