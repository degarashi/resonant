-- オブジェクトハンドルの基本メソッド定義
ObjectBase = {
	_valueR = {
		handleId = HandleBase_handleId,
		numRef = HandleBase_numRef
	},
	_valueW = {},
	_func = {},
	-- C++のリソースハンドル用メタテーブル
	_udata_mt = { __gc = DeleteHandle }
}
-- オブジェクトハンドルの継承
function DerivedHandle(base)
	local object = {}
	local _r, _w, _f = {},{},{}
	local object = {
		_base = base,
		_valueR = _r,
		_valueW = _w,
		_func = _f,
		_mt = {
			__index = function(tbl, key)
				local r = _r[key]
				if r then return r(tbl.udata) end
				r = _f[key]
				if r then
					return function(tbl, ...)
							return r(tbl.udata, ...)
						end
				end
			end,
			__newindex = function(tbl, key, val)
				local r = _w[key]
				if r then
					r(tbl.udata, value)
				end
			end
		}
	}
	-- 各種C++テーブルの継承
	setmetatable(_r, {__index = base._valueR})
	setmetatable(_w, {__index = base._valueW})
	setmetatable(_f, {__index = base._func})
	setmetatable(object, {__index = base})
	return object
end

