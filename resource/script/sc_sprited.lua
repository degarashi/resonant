local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end

local Alg = G.DrawGroup.SortAlg
local N_Sprite = 5
st_idle = {
	OnEnter = function(self, slc, prev)
		G.print("sc_spriteD:OnEnter <-" .. (prev or "nil"))
		self.baseUpd = G.U_UpdGroup.New()
		self.baseDg = G.U_DrawGroup.New({Alg.Priority_Asc}, false)
		slc.objlist = scbase.Init(self.baseUpd, self.baseDg)
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()
		upd:addObj(self.baseUpd)
		dg:setSortAlgorithmId({Alg.Priority_Asc}, false)

		-- FBufferを準備
		local scrSize = System.info:getScreenSize()
		self.hFb = System.glres:makeFBuffer()
		local hDb = System.glres:createTexture(scrSize, G.GLRes.Format.R16, false, false)
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Depth, hDb)
		self.hTex = {}
		for i=1,2 do
			self.hTex[i] = System.glres:createTexture(scrSize, G.GLRes.Format.RGBA8, false, false)
		end
		self.swt = 0
		self.bBlue = false
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Color0, self.hTex[self.swt+1])

		---- Sceneのセットアップ ----
		-- [FBSwitch&ClearZ(Default)]
		do
			local clp = G.ClearParam.New(G.Vec4.New(0.15,0.15,0.15,0), 1.0, nil)
			local fbc = G.FBSwitch.New(0x0000, self.hFb, clp)
			dg:addObj(fbc)
		end
		-- [現シーンのDG] Z値でソート(Dynamic)
		do
			local dgCur = G.U_DrawGroup.New({Alg.Z_Asc}, true)
			dgCur:setPriority(0x1000)
			dg:addObj(dgCur)
			self.dgCur = dgCur
		end
		-- [Blur(前回の重ね)]
		do
			local blur0 = G.BlurEffect.New(0x2000)
			blur0:setAlpha(0)
			dg:addObj(blur0)
			self.blur0 = blur0
		end
		-- [FBSwitch&ClearZ(Default)]
		do
			local clp = G.ClearParam.New(nil, 1.0, nil)
			dg:addObj(G.FBSwitch.New(0x3000, nil, clp))
		end
		-- [Blur(今回のベタ描画)]
		do
			local blur1 = G.BlurEffect.New(0x4000)
			blur1:setAlpha(1.0)
			dg:addObj(blur1)
			self.blur1 = blur1
		end
		-- [HUD]
		self.baseDg:setPriority(0xffff)
		dg:addObj(self.baseDg)

		-- バウンドするスプライトの初期化
		-- ランダムで位置とスピードを設定
		local rnd = Global.cpp.random
		local fnPos = function()
			return G.Vec2.New(rnd:getUniformFloat({-1, 1}),
								rnd:getUniformFloat({-1, 1}))
		end
		local fnSV = function()
			return G.Vec2.New(rnd:getUniformFloat({-0.01, 0.01}),
							rnd:getUniformFloat({-0.005, 0.005}))
		end
		local hTex ={}
		for i=1,N_Sprite do
			hTex[i] = System.glres:loadTexture("spr" .. i-1 .. ".png", G.GLRes.MipState.MipmapLinear, nil)
			hTex[i]:setFilter(true, true)
		end
		for i=0,150-1 do
			local spr = G.U_BoundingSprite.New(self.dgCur, hTex[(i%N_Sprite)+1], fnPos(), fnSV())
			spr:setScale(G.Vec2.New(0.2, 0.2))
			upd:addObj(spr)
		end
	end,
	OnUpdate = function(self, slc, ...)
		if Global.cpp.actNumber0:isKeyPressed() then
			G.print(self.bBlur)
			self.bBlur = not self.bBlur
			self.blur0:setAlpha(self.bBlur and 0.9 or 0.0)
		end
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Color0, self.hTex[self.swt+1])
		self.blur0:setDiffuse(self.hTex[(self.swt~1)+1])
		self.blur1:setDiffuse(self.hTex[self.swt+1])
		self.swt = self.swt ~ 1
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, nextSc)
		G.print("sc_sprite:OnExit ->" .. (nextSc or "nil"))
		if not nextSc then
			scbase.Terminate(slc.objlist)
		end
	end
}
