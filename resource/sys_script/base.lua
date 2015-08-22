require("sysfunc")

-- ObjMgr以外はResource毎に固定
-- ObjMgrは内部でGetLuaName()

-- グローバル変数は全てここに含める
Global = {}
-- ゲームエンジンに関する変数など
-- System = {}
-- HandleUD -> ObjInstance
ObjTypedef = {}
-- ObjMgrはLuaグローバルステートを持つ
-- ObjMgrのreleaseメソッドを改変してカウントがゼロになった時に初めてLuaからテーブルを削除

-- オブジェクト型定義
--[[ \param[in] base 事前にC++で定義しておいたObjectBase
	オブジェクトハンドルの継承 => オブジェクトベースの定義
	これにLuaで記述された各種メソッドを足したものがオブジェクト ]]
function DerivedHandle(base)
	local _r, _w, _f = {},{},{}
	local _mt = {
		__index = function(tbl, key)
			local r = _r[key]
			if r~=nil then
				-- メンバ変数(C++)読み込み
				return r(tbl)
			end
			-- メンバ関数(C++)呼び出し
			return _f[key]
		end,
		__newindex = function(tbl, key, val)
			local w = _w[key]
			if w~=nil then
				-- メンバ変数(C++)書き込み
				w(tbl, val)
			end
			-- メンバ変数(Lua)書き込み
			rawset(tbl, key, val)
		end
	}
	local object = {
		_base = base,
		_valueR = _r,
		_valueW = _w,
		_func = _f,
		_mt = _mt,
		_New = false -- 後で定義する用のダミー
	}
	local instance_mt = {
		__index = _mt.__index,
		__newindex = _mt.__newindex,
		__gc = DecrementHandle
	}
	local instance = {}
	setmetatable(instance, {
		__mode = "v"
	})
	-- (from LCV<SHandle> [C++])
	-- ハンドルIDからLua内のクラスインスタンスを取得
	-- 同一ハンドルなら同じインスタンスを返す
	--[[ \param[in] handle value(light userdata) ]]
	function object.GetInstance(ud)
		assert(type(ud) == "userdata")
		local obj = instance[ud]
		if obj == nil then
			obj = {udata = ud}
			setmetatable(obj, instance_mt)
			instance[ud] = obj
		end
		return obj
	end
	-- ポインタからオブジェクトを構築 (from C++)
	--[[ \param[in] ptr(light userdata) ]]
	function object.ConstructPtr(ptr)
		-- LightUserDataでは個別のmetatableを持てないのでtableで包む
		local ret = {pointer = ptr}
		setmetatable(ret, object._mt)
		return ret
	end

	local makeInstanceAt = function(dst, ud)
		assert(not instance[ud])
		instance[ud] = dst
		dst.udata = ud
		setmetatable(dst, _mt)
	end
	-- selfにはtableを指定する (from [Lua])
	function object.ConstructAt(self, ...)
		-- 初回のオブジェクトインスタンス作成
		local ud = object._New(...)
		makeInstanceAt(self, ud)
	end
	-- 各種C++テーブルの継承
	setmetatable(_r, {__index = base._valueR})
	setmetatable(_w, {__index = base._valueW})
	setmetatable(_f, {__index = base._func})
	return object
end
-- DerivedHandleで基礎を定義した後、C++で_valueR, _valueW, _func, _Newを追加定義する

-- テーブル継承に使うメタテーブルを作成
-- 読み取りは自テーブル、ベーステーブルの順
-- 書き込みは両方が持って無ければ自分に書き込む
local makeDerivedTableMT = function(base)
	return {
		__index = function(tbl, key)
			return base[key]
		end,
		__newindex = function(tbl, key, val)
			local r = base[key]
			if r ~= nil then
				rawset(base, key, val)
			end
			rawset(tbl, key, val)
		end
	}
end

function DerivedState(base, init)
	local state = init
	state._base = base
	setmetatable(state, {
		__index = base,
		__newindex = base
	})
	return state
end

local nilbase = {
	Ctor = function(self) end
}
function DerivedClass(base, init)
	base = base or nilbase
	local object = init
	object._base = base
	object._mt = {
		__index = object,
		__newindex = function(tbl, key, value)
			local r = object[key]
			if r ~= nil then
				rawset(object, key, value)
				return
			end
			r = base[key]
			if r ~= nil then
				rawset(base, key, value)
				return
			end
			rawset(tbl, key, value)
		end
	}

	-- Baseクラスのコンストラクタを呼ぶ
	function object.CallBaseCtor(self, ...)
		base.Ctor(self, ...)
	end
	function object.New(...)
		local obj = {}
		setmetatable(obj, object._mt)
		-- コンストラクタを呼ぶ
		object.Ctor(obj, ...)
		return obj
	end
	-- もし継承先クラスがすでにメタテーブルを持っていたらそれを上乗せ
	local srcMT = getmetatable(object)
	local mt = {
		__index = function(tbl, key)
			return base[key]
		end,
		__newindex = function(tbl, key, value)
			local r = base[key]
			if r ~= nil then
				rawset(base, key, value)
				return
			end
			rawset(tbl, key, value)
		end
	}
	if srcMT ~= nil then
		mt = MakeDerivedMT(srcMT, mt)
	end
	setmetatable(object, mt)
	return object
end

-- ステート遷移ベースクラス
-- self.valueとした場合はインスタンス変数, static変数, c++変数の順
-- ENVに別々
-- メッセージを送ると[現ステート, Baseステート, クラスstatic(C++メソッド)]の順でチェックする
-- C++メソッドにはメッセージをIDで指定するのでまずはGetMessageID("message")を呼んで、有効なIDが返ってきたら
	-- onMessage()
FSMachine = DerivedClass(nil, {
	-- グローバル変数やシステム名はそのまま使えるようにする
	Global = Global,
	System = System,
	--[[
		state(table)		: 現在のステート
		stateS(string)
		nextStateS(string)	: 移行先のステート
		stateLocal(table)	: ステートローカル領域
	]]
	Ctor = function(self, firstState, ...)
		local bmt = _ENV[self.BaseClass].Construct(self, ...)
		local mt = MakeDerivedMT(getmetatable(self), bmt)
		setmetatable(self, mt)

		assert(type(firstState) == "string")
		self.state = self[firstState]
		self.stateS = firstState
		self.stateLocal = {}
		self:RecvMsg("OnEnter", nil)
	end,
	-- \param[in] state(string)	次のステート名
	SetState = function(self, state)
		assert(not self.nextStateS)
		self.nextStateS = state
	end,
	SwitchState = function(self)
		-- ステート変更が全て終わるまでループ
		while self.nextStateS do
			assert(type(self.nextStateS) == "string")

			local nextS = self.nextStateS
			self.nextStateS = nil
			self:RecvMsg("OnExit", nextS)
	
			-- stateLocalを一旦破棄
			self.stateLocal = {}

			local prevS = self.stateS
			self.state = self[nextS]
			self.stateS = nextS
			self:RecvMsg("OnEnter", prevS)
		end
	end,
	-- 全てのメッセージは先頭引数がself, lc(ステートローカル領域)
	-- OnEnter (prevState(string))
	-- OnExit (nextState(string))
	-- OnCollisionEnd (obj(object), nFrame(number))
	-- OnCollision (obj(object), nFrame(number))

	-- \param[in] msg(string)	メッセージ名
	-- \return (bool)受信応答, 任意の戻り値...
	RecvMsg = function(self, msg, ...)
		local rc = self.state[msg]
		if rc then
			local ret = {rc(self, self.stateLocal, ...)}
			-- OnExitでステート変更は不可
			assert(msg ~= "OnExit" or not self.nextStateS)
			self:SwitchState()
			return true, table.unpack(ret)
		end
		-- C++のメッセージとして処理
		local args = {...}
		-- 引数は1つに統合
		local nArg = #args
		if nArg == 0 then
			args = nil
		elseif nArg == 1 then
			args = args[1]
		end
		-- C++で受信した場合は戻り値がnil以外になる
		local func = ObjectBase.RecvMsg
		local ret = func(self, msg, args)
		if ret == nil then
			return false
		end
		return true,ret
	end
})
function MakeDerivedMT(first, second)
	local function makeFuncR(val)
		if type(val) == "table" then
			local tmp = val
			return function(tbl,key)
				return tmp[key]
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
		__gc = first.__gc or second.__gc
	}
end
function MakeFSMachine(src)
	return  DerivedClass(FSMachine, src)
end

--[[	クラス定義ファイルを読む前にこの関数でUpValueにセットする
		(クラス定義ファイル中でグローバル関数を読んだ場合への対処) ]]
-- System = システム関数テーブル
-- Global = 明示的なグローバル変数テーブル
local global,system = Global,System
local print = print
local GTbl = _G
function MakePreENV(base)
	setmetatable(base, {
		__index = function(tbl, key)
			if key == "System" then
				return system
			elseif key == "Global" then
				return global
			end
			return GTbl[key]
		end,
		__newindex = function(tbl, key, val)
			rawset(tbl, key, val)
		end
	})
end

function Test()
	local obj = myobj.New()
	-- 	同期読み込みテスト
	do
		local ret = System:loadResource("file:///tmp/test.jpg")
		RS.PrintValue("sync:", ret)
	end
	local tbl = {
		AAA = "file:///tmp/test.png",
		BBB = "file:///tmp/test.jpg",
		CCC = "file:///tmp/test.png"
	}
	do
		local retT = System:loadResources(tbl)
		RS.PrintValue("sync:", retT)
	end
	-- 非同期読み込みテスト
	do
		local id = System:loadResourcesASync(tbl)
		local num = 0
		repeat
			num = System:queryProgress(id)
			print(num)
		until num >= 1
		RS.PrintValue("async:", System:getResult(id))
	end
end

