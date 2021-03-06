require("vector_common")
local tmp = {
	_size = 4,
	IsVec = function(a)
		return VecC.IsVec(a) and a._size == 4
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
RS.RenameFuncStatic(tmp, "Vec4",
	{"Random", "luaRandom"},
	{"RandomWithLength", "luaRandomWithLength"},
	{"RandomWithAbs", "luaRandomWithAbs"}
)
return tmp
