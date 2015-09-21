#include "fpscamera.hpp"

struct FPSCamera::St_Default : StateT<St_Default> {};
FPSCamera::FPSCamera() {
	setStateNew<St_Default>();
}
