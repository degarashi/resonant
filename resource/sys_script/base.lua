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
		RecvMsg(func)
		Ctor(func)
	}
]]
function DerivedHandle(base, name, object)
	assert(base, "DerivedHandle: base-class is nil")
	assert(type(name)=="string", "DerivedHandle: invalid ObjectName")
	assert(object==nil or type(object)=="table", "DerivedHandle: invalid argument (object)")
	if getmetatable(base) == nil then
		MakePreENV(base)
	end

	local _r, _w, _f = {},{},{}
	local _mt = {
		__index = function(tbl, key)
			-- メンバ変数(C++)読み込み
			local r = _r[key]
			if r~=nil then return r(tbl) end
			-- メンバ関数(C++)呼び出し
			r = _f[key]
			if r~=nil then return r end
			-- ベースクラス読み込み
			return base[key]
		end,
		-- tblにはobjectが渡される
		__newindex = function(tbl, key, val)
			-- ベースクラスへ書き込み
			local w = base[key]
			if w~=nil then
				if type(w) == "function" then
					w(tbl, val)
				else
					base[key] = val
				end
				return
			end
			rawset(tbl, key, val)
		end
	}
	if object == nil then
		object = LoadAdditionalValue(name)
		object._valueR = _r
		object._valueW = _w
		object._func = _f
		object._New = false		-- 後で(C++にて)定義する用のエントリー確保ダミー
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

	-- ポインターインスタンス用のMT = オブジェクトMT + __gc(DecrementHandle)
	local instanceP_mt = {
		__index = object,
		__newindex = object
	}
	-- ハンドル用インスタンスにセットするMT
	local instanceH_mt = {
		__index = object,
		-- tblにはインスタンスが渡される
		__newindex = function(tbl, key, val)
			local w = object[key]
			if w~=nil then
				if type(e) == "function" then
					-- メンバ変数(C++)書き込み
					w(tbl, val)
				else
					object[key] = val
				end
				return
			end
			-- ベースクラスへ書き込み
			w = base[key]
			if w~=nil then
				if type(w) == "function" then
					w(tbl, val)
				else
					base[key] = val
				end
				return
			end
			-- メンバ変数(Lua)書き込み
			rawset(tbl, key, val)
		end,
		__gc = DecrementHandle
	}
	-- インスタンスリスト(弱参照)
	-- HandleUD -> ObjInstance
	local instance = {}
	setmetatable(instance, {
		__mode = "v"
	})
	-- [Public] (from LCV<SHandle> [C++])
	-- ハンドルIDからLua内のクラスインスタンスを取得
	-- 同一ハンドルなら同じインスタンスを返す
	--[[ \param[in] handle value(light userdata) ]]
	function object.GetInstance(ud)
		assert(type(ud) == "userdata")
		local obj = instance[ud]
		if obj == nil then
			obj = {udata = ud}
			setmetatable(obj, instanceH_mt)
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
		assert(not instance[ud])
		instance[ud] = obj
		return obj
	end
	-- [Public] インスタンス作成(from Lua & C++)
	function object.New(...)
		print("----------- New ---------" .. name)
		local obj = object.ConstructNew(...)
		-- コンストラクタを呼ぶ
		obj:Ctor(...)
		return obj
	end
	setmetatable(object, _mt)
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
