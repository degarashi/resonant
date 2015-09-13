require("sysfunc")
local CallOperator = RS.CallOperator
local isVec = function(a)
	return a._postfix == "V"
end
VecC = {}
VecC.Postfix = "V"
VecC.IsVec = isVec
VecC.Metatable = {
	__add = function(a,b)
		assert(a.IsVec(b))
		return a:addV(b)
	end,
	__sub = function(a,b)
		assert(a.IsVec(b))
		return a:subV(b)
	end,
	__mul = function(a,b)
		return CallOperator(a,b, "mul")
	end,
	__div = function(a,b)
		return CallOperator(a,b, "div")
	end,
	__mod = function(a,b)
		assert(a.IsVec(b))
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
