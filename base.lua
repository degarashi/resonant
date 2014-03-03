package.path = package.path .. ";/home/slice/projects/resonant/?.lua"
require("sysfunc")

-- グローバル変数は全てここに含める
Global = {}
-- ゲームエンジンに関する変数など
System = {}
-- HandleID -> ObjInstance(fake)
local handleId2Obj = {}
-- HandleID -> ObjInstance(substance)
local handleId2ObjSub = {}
-- ObjInstance -> HandleID
local ud2HandleId = {}
setmetatable(handleId2Obj, {
	__mode = "v"
})
-- ObjMgrはLuaグローバルステートを持つ
-- ObjMgrのreleaseメソッドを改変してカウントがゼロになった時に初めてLuaからテーブルを削除
-- リソースハンドルの登録・削除関数

-- ハンドルIDからインスタンスを取得
-- From C++(LCV<SHandle>)
function GetHandle(id)
	local h = handleId2Obj[id]
	if h ~= nil then
		return h
	end
	local sub = handleId2ObjSub[id]
	h = {}
	setmetatable(h, {
		__index = sub,
		__newindex = sub,
		__gc = DecrementHandle
	})
	IncrementHandle(sub)
	handleId2Obj[id] = h
	return h
end
-- ObjMgr.release()で参照カウンタがゼロになった際に呼ばれる
function DeleteHandle(ud)
	local id = ud2HandleId[ud]
	assert(id)
	ud2HandleId[ud] = nil
	handleId2Obj[id] = nil
	handleId2ObjSub[id] = nil
end

-- ObjectBaseは事前にC++で定義しておく
-- オブジェクトハンドルの継承 => オブジェクトベースの定義
-- これにLuaで記述された各種メソッドを足したものがオブジェクト
function DerivedHandle(base)
	-- ハンドルを登録 (ユーザーテーブルの作成)
	-- 引数にはClass_New()で生成したmetatable設定済みのUserData
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
				if r~=nil then return r(tbl) end
				return _f[key]
			end,
			__newindex = function(tbl, key, val)
				local r = _w[key]
				if r~=nil then
					r(tbl, value)
				end
				rawset(tbl, key, val)
			end
		},
		_New = false -- 後で定義する用のダミー
	}
	-- selfにはtableを指定する
	function object.Construct(self, ...)
		local ud,id = object._New(...)
		assert(not handleId2ObjSub[id])

		-- 初回のオブジェクトインスタンス作成
		self.udata = ud
		handleId2ObjSub[id] = self
		ud2HandleId[ud] = id
	end
	-- 各種C++テーブルの継承
	setmetatable(_r, {__index = base._valueR})
	setmetatable(_w, {__index = base._valueW})
	setmetatable(_f, {__index = base._func})
	setmetatable(object, object._mt)
	return object
end
-- DerivedHandleで基礎を定義した後、C++で_valueR, _valueW, _func, _Newを追加定義する

-- テーブル継承に使うメタテーブルを作成
-- 読み取りは自テーブル、ベーステーブルの順
-- 書き込みは両方が持って無ければ自分に書き込む
local makeDerivedTableMT = function(base)
	return {
		__index = base,
		__newindex = function(tbl, key, val)
			local r = base[key]
			if r ~= nil then
				rawset(base, key, val)
			end
			rawset(tbl, key, val)
		end
	}
end

local nilbase = {
	Ctor = function(self) end
}
function DerivedClass(base, init)
	base = base or nilbase
	local object = init
	object._mt = makeDerivedTableMT(object)

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

	setmetatable(object, makeDerivedTableMT(base))
	return object
end

-- クラスファイルを読む時にセットする
MT_G = {
	__index = _G,
	__newindex = function(tbl, key, val)
		rawset(tbl, key, val)
	end
}
-- クラスファイルを読んだ後にすり替えるメタテーブルを生成
function MakeStaticValueMT(cls)
	local mt = {
		__index = function(tbl, key)
			-- static変数、global変数の順で調べる
			local v = cls[key]
			if v ~= nil then
				return v
			end
			return _G[key]
		end,
		__newindex = function(tbl, key, val)
			-- static変数だけ調べて、既にエントリがあればそれを上書き
			local v = cls[key]
			if v ~= nil then
				cls[key] = val
			end
			-- 該当なしなら何も書き込まない
			print("invalid data write: key=" .. key .. ", value=" .. val)
		end
	}
	local ret = {}
	setmetatable(ret, mt)
	return ret
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
	]]
	Ctor = function(self, firstState, ...)
		_ENV[self.BaseClass].Construct(self, ...)

-- 		self.nextState = firstState
-- 		self:recvMsg("OnEnter", nil)
	end,
	SetState = function(self, state)
		assert(not self.nextState)
		self.nextState = state
	end,
	SwitchState = function(self)
		while self.nextState do
			self:recvMsg("OnExit", self.nextState)
			local prev = self.state
			self.state = self.nextState
			self.nextState = nil
			self:recvMsg("OnEnter", prev)
		end
	end,
	RecvMsg = function(self, msg, ...)
		local rc = self.state[msg]
		if rc then
			local ret = {rc(self, ...)}
			self:switchState()
			return true, table.unpack(ret)
		end
		return false
	end
})
function MakeDerivedHandleMT(handlebase, base)
	local hmt = getmetatable(handlebase)
	local hmt_r, hmt_w = hmt.__index, hmt.__newindex
	return {
		__index = function(tbl, key)
			local v = base[key]
			if v ~= nil then
				return v
			end
			return hmt_r(tbl, key)
		end,
		__newindex = function(tbl, key, val)
			local r = base[key]
			if r ~= nil then
				rawset(base, key, val)
			end
			hmt_w(tbl, key, val)
		end
	}
end
function MakeFSMachine(src)
	local ret = DerivedClass(FSMachine, src)
	local mt = MakeDerivedHandleMT(_ENV[src.BaseClass], FSMachine)
	setmetatable(ret, mt)
	return ret
end

function Test()
	local obj = myobj.New()
	RS.PrintValue("", type(obj:func(obj)))
end
