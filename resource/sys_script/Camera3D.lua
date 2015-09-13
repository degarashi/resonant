require("vector_common")
require("sysfunc")
return {
	_renamefunc =
		RS.RenameFunc(
			{"setPose", "setPose<spn::Pose3D>"},
			{"setFov", "setFov<spn::RadF>"},
			{"setAspect", "setAspect<float>"},
			{"setNearZ", "setNearZ<float>"},
			{"setFarZ", "setFarZ<float>"}
		)
}
