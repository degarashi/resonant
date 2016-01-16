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

		local tex = System.glres:loadTexture("floor.jpg", G.GLRes.MipState.MipmapLinear, nil)
		tex:setFilter(true, true)
-- 		local sk = G.SkyDome.New(64,64)
-- 		sk:setDrawPriority(0x0800)
-- 		sk:setWidth(100)
-- 		sk:setHeight(10)
		local sk = G.SkyDome.New(0x0800)
		dg:addObj(sk)
		self.sky = sk
		local tf = G.STileField.New(Global.cpp.random,
									64, 16,
									128, 32,
									0.50,
									0, 1)
		tf:setDrawPriority(0x1000)
		tf:setViewPos(G.Vec3.New(0,0,0))
		tf:setViewDistanceCoeff(0, 1)
		tf:setTexture(tex)
		tf:setTextureRepeat(1)
-- 		tf:setViewDir(G.Vec3.New(1,0,0))
		self.tf = tf
		dg:addObj(tf)

		local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
		dg:addObj(G.FBClear.New(0x0000, clp))

		local fc = G.FPSCameraU.New()
		upd:addObj(fc)
		-- RayleighCoeff, MieCoeff
	end,
	OnUpdate = function(self, slc, ...)
		local pose = Global.cpp.hlCam:refPose()
		self.tf:setViewPos(pose:getOffset())
		self.tf:setRayleighCoeff(1)
		self.tf:setMieCoeff(0.76, 1)
		self.tf:setSunDir(G.Vec3.New(0,0.707,-0.707))
		self.tf:setSunColor(G.Vec3.New(1,1,1))
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, ...)
		G.print("sc_cube:OnExit")
		scbase.Terminate(slc.objlist)
	end
}
