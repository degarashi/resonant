#pragma once
#include "../handle.hpp"

class FPSCamera : public rs::ObjectT<FPSCamera> {
	private:
		bool		_bPress;
		spn::DegF	_yaw, _pitch, _roll;
		struct St_Default;
		void initState() override;
	public:
		FPSCamera();
};
