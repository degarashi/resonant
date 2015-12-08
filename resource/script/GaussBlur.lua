require("sysfunc")
return {
	_renamefunc =
		RS.RenameFunc(
			{"setSource<rs::HTex>", "setSource"},
			{"setDest<rs::HTex>", "setDest"},
			{"setDispersion<float>", "setDispersion"}
		)
}
