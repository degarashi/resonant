local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end

local c_name = {
	"z_desc",
	"techpass|z_asc|texture|buffer",
	"techpass|z_desc|texture|buffer",
	"buffer|z_asc|techpass|texture",
	"buffer|z_desc|techpass|texture"
}
local Alg = G.DrawGroup.SortAlg
local c_makeds = {
	{Alg.Priority_Asc, Alg.Z_Desc},
	{Alg.Priority_Asc, Alg.TechPass, Alg.Z_Asc, Alg.Texture, Alg.Buffer},
	{Alg.Priority_Asc, Alg.TechPass, Alg.Z_Desc, Alg.Texture, Alg.Buffer},
	{Alg.Priority_Asc, Alg.Buffer, Alg.Z_Asc, Alg.TechPass, Alg.Texture},
	{Alg.Priority_Asc, Alg.Buffer, Alg.Z_Desc, Alg.TechPass, Alg.Texture}
}
local N_Sprite = 5
st_idle = {
	OnEnter = function(self, slc, prev)
		G.print("sc_sprite:OnEnter <-" .. (prev or "nil"))
		self.baseUpd = G.U_UpdGroup.New()
		self.baseDg = G.U_DrawGroup.New({Alg.Priority_Asc}, false)
		slc.objlist = scbase.Init(self.baseUpd, self.baseDg)
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()
		self.hSpr = {}
		-- スプライト画像の読み込み
		for i=1,N_Sprite do
			local hSt = System.glres:loadTexture("spr" .. i-1 .. ".png", G.GLRes.MipState.MipmapLinear, nil)
			hSt:setFilter(true, true)
			local spr = G.SpriteObj.New(hSt, i*0.1)
			spr:setDrawPriority(0x2000)
			self.hSpr[i] = spr
			upd:addObj(spr)
			dg:addObj(spr)

			spr:setScale(G.Vec2.New(0.3, 0.3))
			spr:setOffset(G.Vec2.New(-1.0 + i*0.2,
									(i&1)*-0.1))
		end
		upd:addObj(self.baseUpd)
		self.baseDg:setDrawPriority(0x1000)
		dg:addObj(self.baseDg)

		local clp = G.ClearParam.New(G.Vec4.New(0,0.15,0,0), 1.0, nil)
		local fbc = G.FBClear.New(0x000, clp)
		dg:addObj(fbc)

		self:SetState("st_test", 1)
	end,
	OnExit = function(self, slc, nextSc)
		G.print("sc_sprite:OnExit ->" .. nextSc)
		if not nextSc then
			scbase.Terminate(slc.objlist)
		end
	end
}
st_test = G.DerivedState(st_init, {
	OnEnter = function(self, slc, prev, test_num)
		G.print("sc_sprite(st_test):OnEnter <-" .. (prev or "nil"))
		local dg = self:getDrawGroup()
		dg:setSortAlgorithmId(c_makeds[test_num], false)
	end,
	OnUpdate = function(self, slc, ...)
		do
			-- 一定時間ごとにランダムなスプライトを選んで一旦グループから外し、再登録
			local index = Global.cpp.random:getUniformInt({1, #self.hSpr})
			local obj = self.hSpr[index]
			local hDg = self:getDrawGroup()
			hDg:remObj(obj)
			hDg:addObj(obj)
		end
		-- 他のテストへ切り替え
		for i=1,#c_makeds do
			if Global.cpp["actNumber" .. i-1]:isKeyPressed() then
				self:SetState("st_test", i)
				return
			end
		end
		scbase.CheckSwitch()
	end,
	GetState = function()
		return "Sprite"
	end
})
