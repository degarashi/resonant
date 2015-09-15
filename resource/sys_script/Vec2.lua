require("vector_common")
require("sysfunc")
local tmp = {
	_size = 2,
	IsVec = function(a)
		return VecC.IsVec(a) and a._size == 2
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
			{"lerp", "l_intp<false>"}
		)
}
RS.RenameFuncStatic(tmp, "Vec2",
	{"Random", "luaRandom"},
	{"RandomWithLength", "luaRandomWithLength"},
	{"RandomWithAbs", "luaRandomWithAbs"}
)
return tmp
