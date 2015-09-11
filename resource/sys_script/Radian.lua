require("sysfunc")
local deg = require("Degree")
local isRadian = function(a)
	return a._postfix == "R"
end
local ret = {
	IsRadian = isRadian,
	_postfix = "R",
	_metatable = {},
	_renamefunc = deg._renamefunc
}
RS.Overwrite(ret._metatable, deg._metatable)
ret._metatable.__lt = function(a,b)
	return a:lessthan(b:toRadian())
end
ret._metatable.__le = function(a,b)
	return a:lessequal(b:toRadian())
end
ret._metatable.__eq = function(a,b)
	return a:equal(b:toRadian())
end
return ret
