require("sysfunc")
require("vector_common")
local CallOperator = RS.CallOperator
local isMat = function(a)
	return a._postfix == "M"
end
MatC = {}
MatC.Postfix = "M"
MatC.Metatable = {
	IsMat = isMat,
	__add = function(a,b)
		assert(VecC.IsVec(b))
		return a:addV(b)
	end,
	__sub = function(a,b)
		assert(VecC.IsVec(b))
		return a:subV(b)
	end,
	__mul = function(a,b)
		return CallOperator(a,b, "mul")
	end,
	__div = function(a,b)
		return CallOperator(a,b, "div")
	end,
	__mod = function(a,b)
		assert(VecC.IsVec(b))
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
