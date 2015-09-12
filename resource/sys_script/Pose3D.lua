return {
	_dim = 3,
	_metatable = {
		__eq = function(a,b)
			return a:equal(b)
		end,
		__tostring = function(a)
			return a:toString()
		end
	}
}
