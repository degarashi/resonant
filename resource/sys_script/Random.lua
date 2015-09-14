require("sysfunc")
return {
	_renamefunc =
		RS.RenameFunc(
			{"getUniformFloat", "luaGetUniform<float>"},
			{"getUniformInt", "luaGetUniform<int>"}
		)
}
