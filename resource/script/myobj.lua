-- C++でベースにするクラス
BaseClass = "Tekito"
-- 初期ステート設定
InitialState = "st_idle"
-- クラスstatic変数。動作中に書き換え可能
static_value = 100
-- クラスメンバ関数
-- function doIt(self)
	-- 基本的にself.xxxはユーザー変数、self.XXXはシステム変数
	-- 自オブジェクトのstatic変数を参照
-- 	static_value = "HELLO"
	-- 他オブジェクトのstatic変数を参照
-- 	local os = System:GetStatic("ObjectName")
-- 	print(os.value)
-- end
-- local print = RS.PrintValue
-- Global.value
-- System.func
-- G.rawget
-- RS.func
-- コンストラクタ
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end

st_idle = {
	OnEnter = function(self, slc, ...)
		G.print("Idle")
		self:CPP()
		self:SetState("st_second")
	end,
	hello = function(self, slc, ...)
		G.print("received", ...)
	end
}
st_second = G.DerivedState(st_idle, {
	OnEnter = function(self, slc, ...)
		G.print("Second")
		self:RecvMsg("hello", 1, 2, 3)
	end
})
--[[
-- ステート定義
st_shared = {
	OnEnter = function (self, lc)
		-- オブジェクトをグループで管理したければココで登録したりする
		Global.enemy[#Global.enemy+1] = self
		-- または、名前で登録
		Global.the_object = self
		-- クラスローカル変数初期化
		self.value = 1000
		self.msg = "hello"
		-- ステートローカル変数(ステート切替で破棄)
		lc.value = 1000
		lc.tekito = 200
		-- ステート遷移。一度に2つ以上指定するとエラー
		self:SetState("st_move")
		-- クラスstatic変数の参照
		print ("this is static value" .. self.static_value)
		self:doIt()
		-- static変数の書き換え
		self.static_value = 1000
		-- 当たり判定の登録
		Collision:addA(self)
		-- 当たり判定の削除
		Collision:remA(self)
	end,
	-- 各種システムメッセージに対応する処理など定義
	OnCollisionEnter = function(self, lc, obj)
	end,
	OnCollisionEnd = function(self, lc, obj, n)
	end,
	OnCollision = function(self, lc, obj, n)
	end,
	-- メッセージに対する処理。戻り値は1: bool=受信したか, 2:ユーザー任意(C++の場合は1つのみ)
	UserMessage = function(self, lc, ...)
		-- 1: ベースクラス名
		-- 2: スクリプトファイル名から拡張子を除いた物
		System:CreateObject("Bullet", "normal_bullet")
		-- 効果音を鳴らす (サウンドマネージャを使う)
		Global.sound:play("shot.ogg", 1.0, 0)
		-- 明示的にベースメソッドを呼ぶ
		self:CallBase(lc, next)
		-- 実際にユーザーが返すのは1つ
		return 100
	end
}
-- 継承ステート定義
st_idle = DerivedFrom(st_shared, {
	OnEnter = function(self, lc, prev)
	end,
	OnExit = function(self, lc, next)
		-- 一度マニュアルでベースメソッドを呼ぶか、trueを返すとベースメソッドは呼ばれない
		-- SetStateがダブるとエラー
		return self:CallBase(lc, next)
	end,
})
]]
