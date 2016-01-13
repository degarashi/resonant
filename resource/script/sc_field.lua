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
		local tf = G.TileField.New(Global.cpp.random,
									512, 32,
									128, 64,
									0.45,
									0, 1)
		tf:setDrawPriority(0x1000)
		tf:setViewPos(G.Vec3.New(0,0,0))
		tf:setViewDistanceCoeff(0.1, 1.0)
		tf:setTexture(tex)
		tf:setTextureRepeat(1)
-- 		tf:setViewDir(G.Vec3.New(1,0,0))
		self.tf = tf
		dg:addObj(tf)

		local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1, nil)
		dg:addObj(G.FBClear.New(0x0000, clp))

		local fc = G.FPSCameraU.New()
		upd:addObj(fc)
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
