require("matrix_common")
local isPlane = function(a)
	return a._postfix == "P"
end
return {
	IsPlane = isPlane,
	_postfix = "P",
	_metatable = {
		__mul = function(a,b)
			assert(MatC.IsMat(b))
			return a:mulM(b)
		end,
		__eq = function(a,b)
			assert(isPlane(b))
			return a:equal(b)
		end,
		__tostring = function(a)
			return a:toString()
		end
	}
}
