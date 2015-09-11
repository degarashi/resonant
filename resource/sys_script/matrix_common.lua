require("sysfunc")
local CallOperator = RS.CallOperator
MatC = {}
MatC.Postfix = "M"
MatC.Metatable = {
	__add = function(a,b)
		assert(type(b) == "table")
		return a:addV(b)
	end,
	__sub = function(a,b)
		assert(type(b) == "table")
		return a:subV(b)
	end,
	__mul = function(a,b)
		return CallOperator(a,b, "mul")
	end,
	__div = function(a,b)
		return CallOperator(a,b, "div")
	end,
	__mod = function(a,b)
		assert(type(b) == "table")
		return a:modV(b)
	end,
	__unm = function(a)
		return a:invert()
	end,
	__len = function(a)
		return a:length()
	end,
	__eq = function(a,b)
		return a:equal(b)
	end,
	__tostring = function(a)
		return a:toString()
	end
}
