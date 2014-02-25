-- グローバル変数は全てここに含める
Global = {}
-- システム関数はRSに纏める
RS = {
	Clone = function() end
}
-- HandleID -> ObjInstance
local handleId2Obj = {}
local ud2HandleId = {}
setmetatable(handleId2Obj, {
	__mode = "v"
})
setmetatable(ud2HandleId, {
	__mode = "k"
})
-- ObjMgrはLuaグローバルステートを持つ
-- ObjMgrのreleaseメソッドを改変してカウントがゼロになった時に初めてLuaからテーブルを削除
-- リソースハンドルの登録・削除関数
-- 以下はC++から呼ばれる
-- ハンドルIDからインスタンスを取得
function GetHandle(id)
	return handleId2Obj[id]
end
-- Lua(Object.New())から呼ばれる
-- ハンドルを登録 (ユーザーテーブルの作成)
-- 引数にはClass_New()で生成したmetatable設定済みのUserData
function RegHandle(id, ud)
	assert(not handleId2Obj[id])

	local tbl = {udata = ud}
	handleId2Obj[id] = tbl
	ud2HandleId[tbl] = id
	return tbl
end
-- ObjMgr.release()で参照カウンタがゼロになった際に呼ばれる
function DeleteHandle(ud)
	local id = ud2HandleId[ud]
	assert(id)
	ud2HandleId[ud] = nil
	handleId2Obj[id] = nil
end

-- ObjectBaseは事前にC++で定義しておく
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
-- DerivedHandleで基礎を定義した後、C++で_valueR, _valueW, _func, Newを追加定義する

