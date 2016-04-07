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

		Global.cpp.hlCam:setFarZ(1000)
		Global.cpp.hlCam:setNearZ(2)

		-- FBufferを準備
		local scrSize = System.info:getScreenSize()
		local cbuff = System.glres:createTexture(scrSize, G.GLRes.Format.RGB16F, false, false)
		cbuff:setFilter(true, true)
		self.hFb = System.glres:makeFBuffer()
		self.hFb:attachTexture(G.GLFBuffer.Attribute.Color0, cbuff)
		local hDb = System.glres:makeRBuffer(scrSize[1], scrSize[2], G.GLRes.Format.DEPTH_COMPONENT16)
		self.hFb:attachRBuffer(G.GLFBuffer.Attribute.Depth, hDb)

		-- カメラの初期化
		do
			local fc = G.FPSCameraU.New()
			upd:addObj(fc)
		end

		do
			local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
			local fbc = G.FBSwitch.New(0x00000, self.hFb, clp)
			dg:addObj(fbc)
		end

		-- Clipmapの初期化
		local clip = G.ClipmapObj.New(64, 7, 0)
		clip:setDrawPriority(0x6000)
		clip:setCamera(Global.cpp.hlCam)
		dg:addObj(clip)

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
		-- -- Dawn --
		-- param = {
		-- 	scale = {8,1},
		-- 	divide = 1e3,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.75,
		-- 	mie = 0.3,
		-- 	lpower = 4.8,
		-- 	ldir = G.Vec3.New(0, 0, -1)
		-- }
		-- -- -- Twilight --
		-- param = {
		-- 	scale = {1,1},
		-- 	divide = 9e3,
		-- 	rayleigh = G.Vec3.New(680*1e-3, 550*1e-3, 450*1e-3),
		-- 	mie_gain = 0.8,
		-- 	mie = 0.5,
		-- 	lpower = 2,
		-- 	ldir = G.Vec3.New(0, 0, -1)
		-- }
		clip:setDivide(param.divide)
		clip:setRayleighCoeff(param.rayleigh)
		clip:setMieCoeff(param.mie_gain, param.mie)
		clip:setLightPower(param.lpower)
		clip:setLightDir(param.ldir)
		clip:setGridSize(4,64)
		clip:setDiffuseSize(2,2)

		-- バッファ内容を表示
		-- do
		-- 	local blur0 = G.BlurEffect.New(0x50000)
		-- 	blur0:setAlpha(1)
		-- 	blur0:setDiffuse(clip:getCache()[2])
		-- 	blur0:setRect({-0.5,0,-1,-0.5})
		-- 	dg:addObj(blur0)
		-- end
		-- do
		-- 	local blur0 = G.BlurEffect.New(0x50001)
		-- 	blur0:setAlpha(1)
		-- 	blur0:setDiffuse(clip:getCache()[1])
		-- 	blur0:setRect({-1,-0.5,-1,-0.5})
		-- 	dg:addObj(blur0)
		-- end

		-- Daylight --
		do
			param.divide = 2e1
		end
		-- -- -- Twilight --
		-- do
		-- 	param.divide = 8e1
		-- 	param.lpower = 5
		-- end

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

		local tm
		do
			tm = G.ToneMap.New(0x020000)
			tm:setSource(cbuff)
			dg:addObj(tm)
			self.hdrResult = tm:getResult()
		end
		do
			local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
			local fbc = G.FBSwitch.New(0x030000, nil, clp)
			dg:addObj(fbc)
		end
		do
			local blur0 = G.BlurEffect.New(0x040000)
			blur0:setAlpha(1)
			blur0:setDiffuse(self.hdrResult)
			dg:addObj(blur0)
		end
	end,
	OnUpdate = function(self, slc, ...)
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, ...)
		G.print("sc_cube:OnExit")
		scbase.Terminate(slc.objlist)
	end
}
