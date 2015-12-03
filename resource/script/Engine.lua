return {
	_renamefunc =
		RS.RenameFunc(
			{"setLightPosition", "setLightPosition<const spn::Vec3&>"},
			{"setLightColor", "setLightColor<const spn::Vec3&>"},
			{"setLightDir", "setLightDir<const spn::Vec3&>"},
			{"setLightPower", "setLightPower<float>"},
			{"setLightDepthSize", "setLightDepthSize<const spn::Size&>"},
			{"setDepthRange", "setDepthRange<const spn::Vec2&>"}
		)
}