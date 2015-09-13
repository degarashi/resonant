require("sysfunc")
local CallOperator = RS.CallOperator
local isQuat = function(a)
	return a._postfix == "Q"
end
return {
	IsQuat = isQuat,
	_postfix = "Q",
	_metatable = {
		__add = function(a,b)
			assert(isQuat(b))
			return a:addQ(b)
		end,
		__sub = function(a,b)
			assert(isQuat(b))
			return a:subQ(b)
		end,
		__mul = function(a,b)
			return CallOperator(a,b, "mul")
		end,
		__div = function(a,b)
			assert(type(b) == "number")
			return a:divF(b)
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
}
