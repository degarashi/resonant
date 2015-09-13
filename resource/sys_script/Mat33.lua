require("matrix_common")
return {
	_size = 3,
	IsMat = function(a)
		return MatC.IsMat(a) and a._size == 3
	end,
	_postfix = MatC.Postfix,
	_metatable = MatC.Metatable
}
