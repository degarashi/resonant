BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end

local scbase = G.require("sc_base")
st_idle = {
	OnEnter = function(self, slc, ...)
		slc.objlist = scbase.Init(self:getUpdGroup(), self:getDrawGroup())
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()

		-- FBufferを準備
		local scrSize = System.info:getScreenSize()
		local cbuff = System.glres:createTexture(scrSize, G.GLRes.Format.RGB16F, false, false)
		cbuff:setFilter(true, true)
		self.hFb = System.glres:makeFBuffer()
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Color0, cbuff)
		local hDb = System.glres:makeRBuffer(scrSize[1], scrSize[2], G.GLRes.Format.DEPTH_COMPONENT16)
		self.hFb:attachRBuffer(G.GLFBuffer.Attribute.Depth, hDb)
		
		do
			local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
			local fbc = G.FBSwitch.New(0x00000, self.hFb, clp)
			dg:addObj(fbc)
		end
		local param
		-- Daylight --
		param = {
			scale = {1,1},
			divide = 2e1,
			rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
			mie_gain = 0.9,
			mie = 0.1,
			lpower = 24.4,
			ldir = G.Vec3.New(0, -1, 0)
		}
		-- -- -- Dawn --
		-- param = {
		-- 	scale = {8,1},
		-- 	divide = 2e3,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.75,
		-- 	mie = 0.3,
		-- 	lpower = 4.8,
		-- 	ldir = G.Vec3.New(0, 0, -1)
		-- }
		-- -- -- Hazy --
		-- param = {
		-- 	scale = {1,1},
		-- 	divide = 1e1,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.3,
		-- 	mie = 80,
		-- 	lpower = 4.8,
		-- 	ldir = G.Vec3.New(0, -1, -1):normalization()
		-- }
		-- -- -- Twilight --
		-- param = {
		-- 	scale = {1,1},
		-- 	divide = 8e1,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.8,
		-- 	mie = 0.5,
		-- 	lpower = 9,
		-- 	ldir = G.Vec3.New(0, 0, -1)
		-- }
		-- -- Night --
		-- param = {
		-- 	scale = {1,1},
		-- 	divide = 1e1,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.3,
		-- 	mie = 0.5,
		-- 	lpower = 0.1,
		-- 	ldir = G.Vec3.New(0, -1, 0)
		-- }
		do
			local sk = G.SkyDome.New(0x0800)
			dg:addObj(sk)
			self.sky = sk

			sk:setScale(param.scale[1], param.scale[2])
			sk:setDivide(param.divide)
			sk:setRayleighCoeff(param.rayleigh)
			sk:setMieCoeff(param.mie_gain, param.mie)
			sk:setLightPower(param.lpower)
			sk:setLightDir(param.ldir)
		end
		do
			local tex = System.glres:loadTexture("floor.jpg", G.GLRes.MipState.MipmapLinear, nil)
			tex:setFilter(true, true)
			local tf = G.STileField.New(Global.cpp.random,
										64, 16,
										128, 32,
										0.50,
										0, 1)
			tf:setDrawPriority(0x01000)
			tf:setViewPos(G.Vec3.New(0,0,0))
			tf:setViewDistanceCoeff(0, 1)
			tf:setTexture(tex)
			tf:setTextureRepeat(1)

			tf:setDivide(param.divide)
			tf:setRayleighCoeff(param.rayleigh)
			tf:setMieCoeff(param.mie_gain, param.mie)
			tf:setLightPower(param.lpower)
			tf:setLightDir(param.ldir)
			self.tf = tf
			dg:addObj(tf)
		end
		do
			local fc = G.FPSCameraU.New()
			upd:addObj(fc)
		end

		local tm2
		do
			local tm = G.ToneMap.New(0x02000)
			tm:setSource(cbuff)
			dg:addObj(tm)
			self.hdrResult = tm:getResult()
			tm2 = tm
		end
		do
			local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
			local fbc = G.FBSwitch.New(0x03000, nil, clp)
			dg:addObj(fbc)
		end
		do
			local blur0 = G.BlurEffect.New(0x04000)
			blur0:setAlpha(1)
			blur0:setDiffuse(self.hdrResult)
			dg:addObj(blur0)
		end
		do
			local blur0 = G.BlurEffect.New(0x05000)
			blur0:setAlpha(1)
			blur0:setDiffuse(tm2:getShrink0())
			blur0:setRect({-1,-0.5,-1,-0.5})
			dg:addObj(blur0)
		end
		do
			local blur0 = G.BlurEffect.New(0x05000)
			blur0:setAlpha(1)
			blur0:setDiffuse(tm2:getShrinkA()[2])
			blur0:setRect({-0.5,0,-1,-0.5})
			dg:addObj(blur0)
		end
	end,
	OnUpdate = function(self, slc, ...)
		local pose = Global.cpp.hlCam:refPose()
		self.tf:setViewPos(pose:getOffset())
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, ...)
		G.print("sc_cube:OnExit")
		scbase.Terminate(slc.objlist)
	end
}
