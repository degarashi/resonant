require("sysfunc")

-- グローバル変数は全てここに含める
Global = {}
-- ゲームエンジンに関する変数など
System = {}

do
	--[[	クラス定義ファイルを読む前にこの関数でUpValueにセットする
			(クラス定義ファイル中でグローバル関数を読んだ場合への対処) ]]
	-- Global = 明示的なグローバル変数テーブル
	-- System = システム関数テーブル
	-- RS = 自作エンジンの雑多な関数
	-- G = 本来のグローバル環境
	local global, system, g_tbl, rs_tbl = Global, System, _G, RS
	local sys_mtf = function(tbl, key)
		if key == "System" then
			return system
		elseif key == "Global" then
			return global
		elseif key == "G" then
			return g_tbl
		elseif key == "RS" then
			return rs_tbl
		end
	end
	function MakePreENV(base)
		setmetatable(base, {
			__index = sys_mtf
		})
	end
end

-- インスタンスリスト(弱参照)
-- HandleUD -> ObjInstance
local instance = {}
setmetatable(instance, {
	__mode = "v"
})
-- インスタンスリスト(強参照)
-- (Luaで生成したインスタンスはC++側のハンドルが削除される時まで保持する)
-- HandleUD -> ObjInstance
local lua_instance = {}
function DeleteHandle(id)
	lua_instance[id] = nil
end

-- クラスのEnum値などを（あれば）読み込み
function LoadAdditionalValue(modname)
	-- モジュールが見つからなかった場合は空のテーブルを返すようにする
	local ps = package.searchers
	ps[#ps+1] = function(...)
		return	function()
					return {}
				end
	end
	local ret = require(modname)
	-- 元に戻しておく
	ps[#ps] = nil
	return ret
end

-- オブジェクト型定義
--[[ \param[in] base 事前にC++で定義しておいたObjectBase
	これにLuaで記述された各種メソッドを足したものがゲームオブジェクト
	ObjectBase {
		_valueR = {HandleId(func), NumRef(func)}
		_valueW = {}
		_func = {}
		classname = "ObjectName"
		RecvMsg(func) = RecvMsgCpp
		RecvMsgCpp(func)
		Ctor(func)
	}
]]
function IsObjectInitialized(obj, name)
	return obj._name == name
end
function __Index(base, tbl, key)
	local r = rawget(base, key)
	if r ~= nil then
		return r
	end
	-- メンバ変数(C++)読み込み
	r = base._valueR[key]
	if r~=nil then
		return r(tbl)
	end
	-- メンバ関数(C++)呼び出し
	r = base._func[key]
	if r~=nil then
		return r
	end
	-- ベースクラス読み込み
	base = base._base
	if base then
		return __Index(base, tbl, key)
	end
end
function __NewIndex(base, tbl, key, val)
	local w = rawget(base, key)
	if w~=nil then
		rawset(base, key, val)
		return
	end
	-- メンバ変数(C++)書き込み
	w = base._valueW[key]
	if w~=nil then
		w(tbl, val)
		return
	end
	rawset(tbl, key, val)
end
local MakeHandleWrap = function(obj)
	local ret = {}
	setmetatable(ret, {
		__index = obj,
		__newindex = obj,
		__gc = DecrementHandle
	})
	return ret
end
function DerivedHandle(base, name, object, bNoLoadValue)
	assert(base, "DerivedHandle: base-class is nil")
	assert(type(name)=="string", "DerivedHandle: invalid ObjectName")
	assert(object==nil or type(object)=="table", "DerivedHandle: invalid argument (object)")
	if IsObjectInitialized(object, name) then
		return object
	end
	if getmetatable(base) == nil then
		MakePreENV(base)
	end

	object = object or {}
	local _r, _w, _f = {},{},{}
	local _mt = {
		__index = function(tbl, key)
			return __Index(object, tbl, key)
		end,
		__newindex = function(tbl, key, val)
			__NewIndex(object, tbl, key, val)
		end
	}
	if not bNoLoadValue then
		RS.Overwrite(object, LoadAdditionalValue(name))
		local mt = object._metatable
		if mt then
			RS.Overwrite(_mt, mt)
		end
		local rf = object._renamefunc
		if rf then
			RS.Overwrite(_f, rf)
		end
		object._valueR = _r
		object._valueW = _w
		object._func = _f
		object._pointer = false
		object._New = false		-- 後で(C++にて)定義する用のエントリー確保ダミー(メタテーブルの関係)
	end
	if object.Ctor==nil then
		-- [Protected] 空のコンストラクタを用意
		function object.Ctor(self, ...)
			print("object.default Ctor:" .. object._name)
			base.Ctor(self, ...)
		end
	end
	object._name = name
	object._base = base

	-- ポインターインスタンス用のMT = オブジェクトMT
	local instanceP_mt = _mt
	-- ハンドル用インスタンスにセットするMT
	local instanceH_mt = _mt
	-- [Public] (from LCV<SHandle> [C++])
	-- ハンドルIDからLua内のクラスインスタンスを取得
	-- 同一ハンドルなら同じインスタンスを返す
	--[[ \param[in] handle value(light userdata) ]]
	function object.GetInstance(ud)
		assert(type(ud) == "userdata")
		local obj = instance[ud]
		if obj == nil then
			obj = lua_instance[ud]
			if obj == nil then
				obj = {udata = ud}
				setmetatable(obj, instanceH_mt)
				lua_instance[ud] = obj
			end
			obj = MakeHandleWrap(obj)
			instance[ud] = obj
			IncrementHandle(obj)
		end
		return obj
	end
	-- [Public] ポインタからオブジェクトを構築 (from C++)
	--[[ \param[in] ptr(light userdata) ]]
	function object.ConstructPtr(ptr)
		-- LightUserDataでは個別のmetatableを持てないのでtableで包む
		local ret = {pointer = ptr}
		setmetatable(ret, instanceP_mt)
		return ret
	end
	-- [Private] Lua & C++からオブジェクトを構築(C++のハンドル確保部分)
	function object.ConstructNew(...)
		-- 初回のオブジェクトインスタンス(C++)作成
		local ud = object._New(...)
		local obj = {udata = ud}
		setmetatable(obj, instanceH_mt)
		lua_instance[ud] = obj

		obj = MakeHandleWrap(obj)
		assert(not instance[ud])
		instance[ud] = obj
		return obj
	end
	-- [Public] インスタンス作成(from Lua & C++)
	function object.New(...)
		if object._pointer then
			local ret = object._New(...)
			setmetatable(ret, instanceP_mt)
			return ret
		end
		local obj = object.ConstructNew(...)
		-- コンストラクタを呼ぶ
		obj:Ctor(...)
		return obj
	end
	setmetatable(object, {
		__index = function(tbl, key)
			local base = tbl._base
			if base then
				return base[key]
			end
		end,
		__newindex = function(tbl, key, val)
			local r = base[key]
			if r ~= nil then
				base[key] = val
				return
			end
			rawset(tbl, key, val)
		end
	})
	return object
end

-- 2つのメタテーブルをマージ (firstが優先)
function MakeDerivedMT(first, second)
	local function makeFuncR(val)
		if type(val) == "table" then
			return function(tbl,key)
						return val[key]
					end
		elseif type(val) == "nil" then
			return function(tbl,key) end
		end
		return val
	end
	local function makeFuncW(val)
		if type(val) == "table" then
			local tmp = val
			return function(tbl,key,val)
						tmp[key] = val
					end
		elseif type(val) == "nil" then
			return function(tbl,key,val) end
		end
		return val
	end
	local fmt_r,fmt_w = makeFuncR(first.__index), makeFuncW(first.__newindex)
	local smt_r,smt_w = makeFuncR(second.__index), makeFuncW(second.__newindex)
	local fgc,sgc = first.__gc, second.__gc
	local gc
	if fgc or sgc then
		if not sgc then
			gc = fgc
		end
		if not fgc then
			gc = sgc
		end
		if fgc and sgc then
			gc = function(obj)
				fgc(obj)
				sgc(obj)
			end
		end
	end
	-- Read, Write共にFirst, Secondの順で検索
	return {
		__index = function(tbl, key)
			local v = fmt_r(tbl, key)
			if v ~= nil then
				return v
			end
			return smt_r(tbl,key)
		end,
		__newindex = function(tbl, key, val)
			local r = fmt_r(tbl, key)
			if r ~= nil then
				fmt_w(tbl, key, val)
				return
			end
			r = smt_r(tbl, key)
			if r ~= nil then
				smt_w(tbl, key, val)
				return
			end
			rawset(tbl, key, val)
		end,
		__gc = gc
	}
end
