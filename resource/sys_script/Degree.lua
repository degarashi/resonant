require("sysfunc")
local CallOperator = RS.CallOperator
local isDegree = function(a)
	return a._postfix == "D"
end
return {
	IsDegree = isDegree,
	_postfix = "D",
	_metatable = {
		__add = function(a,b)
			return CallOperator(a,b, "add")
		end,
		__sub = function(a,b)
			return CallOperator(a,b, "sub")
		end,
		__mul = function(a,b)
			assert(type(b) == "number")
			return a:mulF(b)
		end,
		__div = function(a,b)
			assert(type(b) == "number")
			return a:divF(b)
		end,
		__unm = function(a)
			return a:invert()
		end,
		__lt = function(a,b)
			return a:lessthan(b:toDegree())
		end,
		__le = function(a,b)
			return a:lessequal(b:toDegree())
		end,
		__eq = function(a,b)
			return a:equal(b:toDegree())
		end,
		__tostring = function(a)
			return a:toString()
		end
	},
	_renamefunc =
		RS.RenameFunc(
			{"addD", "luaAddD"},
			{"addR", "luaAddR"},
			{"subD", "luaSubD"},
			{"subR", "luaSubR"},
			{"mulF", "luaMulF"},
			{"divF", "luaDivF"},
			{"invert", "luaInvert"},
			{"toDegree", "luaToDegree"},
			{"toRadian", "luaToRadian"},
			{"lessthan", "luaLessthan"},
			{"lessequal", "luaLessequal"},
			{"equal", "luaEqual"},
			{"toString", "luaToString"}
		)
}
