local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self.bCube = false
	self._base.Ctor(self, InitialState, ...)
end
function InitScene(self)
	local upd,dg = self:getUpdGroup(), self:getDrawGroup()
	local engine = Global.engine
	engine:setDispersion(3.7)
	engine:setLineLength(0.05)
	engine:clearScene()
	-- ライト深度バッファサイズ指定
	engine:setLightDepthSize({512,512})
	do
		if self.prevDS then
			dg:remObj(self.prevDS)
		end
		local ds
		if self.bCube then
			ds = engine:getCubeScene(0x1000)
		else
			ds = engine:getDLScene(0x1000)
		end
		dg:addObj(ds)
		self.prevDS = ds
	end
	local addObj = function(size, tex, texNml, typ, bFlat, bFlip, pos, bRotate)
		local obj = G.PrimitiveObj.New(size, tex, texNml, typ, bFlat, bFlip)
		obj:setOffset(pos)
		obj:setDrawPriority(0x1000)
		engine:addSceneObject(obj)
		if bRotate then
			self.rotate[#self.rotate+1] = obj
		end
	end
	self.rotate = {}
	local texBlock = System.glres:loadTexture("block.jpg", G.GLRes.MipState.MipmapLinear, nil)
	local texBlockNml = System.glres:loadTexture("block_normal.png", G.GLRes.MipState.MipmapLinear, nil)
	local texFloor = System.glres:loadTexture("floor.jpg", G.GLRes.MipState.MipmapLinear, nil)
	texBlock:setFilter(true, true)
	texBlockNml:setFilter(true, true)
	texFloor:setFilter(true, true)
	addObj(1.0, texBlock, texBlockNml, G.PrimitiveObj.Type.Torus, false, false,
			G.Vec3.New(0,0,2), true)
	addObj(6.0, texFloor, nil, G.PrimitiveObj.Type.Cube, true, true,
			G.Vec3.New(0,0,2), false)
	addObj(0.5, texBlock, texBlockNml, G.PrimitiveObj.Type.Cube, true, false,
			G.Vec3.New(-4,-4,-3), true)
	addObj(1.0, texBlock, texBlockNml, G.PrimitiveObj.Type.Sphere, false, false,
			G.Vec3.New(2,-3,0), true)
	addObj(2.0, texBlock, texBlockNml, G.PrimitiveObj.Type.Cone, true, false,
			G.Vec3.New(-3,3,4), true)
end
st_idle = {
	OnEnter = function(self, slc, ...)
		G.print("sc_cube:OnEnter")
		slc.objlist = scbase.Init(self:getUpdGroup(), self:getDrawGroup())
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()
		do
			local fc = G.FPSCameraU.New()
			upd:addObj(fc)
		end

		-- バイラテラル係数表示
		do
			local t = G.TextShow.New(0x8000)
			t:setOffset(G.Vec2.New(0, 400))
			t:setBGDepth(0.1)
			t:setBGColor(G.Vec3.New(0,0,1))
			dg:addObj(t)
			self.text = t
		end
		-- FBufferを準備
		local scrSize = System.info:getScreenSize()
		self.hFb = System.glres:makeFBuffer()
		local hDb = System.glres:makeRBuffer(scrSize[1], scrSize[2], G.GLRes.Format.DEPTH_COMPONENT16)
		self.hFb:attachRBuffer(G.GLFBuffer.Attribute.Depth, hDb)
		local hTex = System.glres:createTexture(scrSize, G.GLRes.Format.RGB8, false, false)
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Color0, hTex)
		hTex:setFilter(true, true)
		local engine = Global.engine
		engine:setOutputFramebuffer(self.hFb)

		do
			local bl0 = G.BilateralBlur.New(0x6000)
			self.bl0 = bl0
			slc.gdisp = 50
			slc.bdisp = 0.001
			bl0:setSource(hTex)
			bl0:setDest(hTex)
			bl0:setTmpFormat(G.GLRes.Format.RGBA16F)
			dg:addObj(bl0)
		end
		do
			local blur0 = G.BlurEffect.New(0x7000)
			blur0:setAlpha(1)
			blur0:setDiffuse(hTex)
			dg:addObj(blur0)
		end
		do
			local clp = G.ClearParam.New(nil, nil, nil)
			local fbc = G.FBSwitch.New(0x2000, self.hFb, clp)
			dg:addObj(fbc)
		end
		do
			local clp = G.ClearParam.New(nil, nil, nil)
			local fbc = G.FBSwitch.New(0x5000, nil, clp)
			dg:addObj(fbc)
		end
		self.makeLight = function(self, h, a, color)
			local lorigin = G.Vec3.New(0,h, 2)
			local ldirorigin = G.Vec3.New(0,h, 2)
			local radius = G.Vec3.New(5, 0.5, 5)
			local freq = G.Vec3.New(2,1,2)
			local angle = G.Degree.New(a)
			local lit = G.RotLight.New(lorigin,
										ldirorigin,
										radius,
										freq,
										angle,
										color,
										dg)
			upd:addObj(lit)
		end
		self:makeLight(-4.5, 0, G.Vec3.New(1,0,0))
		self:makeLight(-3.5, 15, G.Vec3.New(0,1,0))
		self:makeLight(-2.5, 30, G.Vec3.New(0,0,1))
		self:makeLight(-1.5, 45, G.Vec3.New(1,0,1))
		self:makeLight(-0.5, 60, G.Vec3.New(0,1,1))
		self:makeLight(0.5, 75, G.Vec3.New(1,1,1))
		self.hTex = {}

		self.bCube = false
		self:InitScene()
	end,
	OnEffectReset = function(self, slc, ...)
		self:InitScene()
	end,
	OnUpdate = function(self, slc, ...)
		-- フィルタ係数変化 --
		local g = Global.cpp
		local bdiff,gdiff = 0,0
		if g.actFilterUpG:isKeyPressing() then
			gdiff = 0.1
		elseif g.actFilterDownG:isKeyPressing() then
			gdiff = -0.1
		end
		if g.actFilterUpB:isKeyPressing() then
			bdiff = 0.01
		elseif g.actFilterDownB:isKeyPressing() then
			bdiff = -0.01
		end
		if bdiff ~= 0 or gdiff ~= 0 then
			slc.bdisp = slc.bdisp + bdiff
			slc.gdisp = slc.gdisp + gdiff
			self.bl0:setGDispersion(slc.gdisp)
			self.bl0:setBDispersion(slc.bdisp)
			local str = "Bilateral Coeff:" .. slc.bdisp .. "\n"
			str = str .. "Gauss Coeff:" .. slc.gdisp
			self.text:setText(str)
		end
		Global.engine:setDispersion(400)
		Global.engine:setOffset(0.1)

		-- スポットライトと点光源を切り替え
		if Global.cpp.actScene:isKeyPressed() then
			self.bCube = not self.bCube
			self:InitScene()
		end

		-- キューブを回転
		for i=1,#self.rotate do
			self.rotate[i]:advance(1.1)
		end
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, ...)
		G.print("sc_cube:OnExit")
		scbase.Terminate(slc.objlist)
	end,
	GetState = function()
		return "Cube"
	end
}
