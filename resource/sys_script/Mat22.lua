require("matrix_common")
return {
	_size = 2,
	IsMat = function(a)
		return MatC.IsMat(a) and a._size == 2
	end,
	_postfix = MatC.Postfix,
	_metatable = MatC.Metatable
}
