require("vector_common")
require("sysfunc")
return {
	_size = 3,
	IsVec = function(a)
		return VecC.IsVec(a) and a._size == 3
	end,
	_postfix = VecC.Postfix,
	_metatable = VecC.Metatable,
	_renamefunc =
		RS.RenameFunc(
			{"dot", "dot<false>"},
			{"distance", "distance<false>"},
			{"getMin", "getMin<false>"},
			{"selectMin", "selectMin<false>"},
			{"getMax", "getMax<false>"},
			{"selectMax", "selectMax<false>"},
			{"lerp", "l_intp<false>"},
			{"random", "luaRandom"},
			{"randomWithLength", "luaRandomWithLength"},
			{"randomWithAbs", "luaRandomWithAbs"}
		)
}
