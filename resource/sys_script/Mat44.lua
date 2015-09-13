require("matrix_common")
return {
	_size = 4,
	IsMat = function(a)
		return MatC.IsMat(a) and a._size == 4
	end,
	_postfix = MatC.Postfix,
	_metatable = MatC.Metatable
}
