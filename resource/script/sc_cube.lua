local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end
st_idle = {
	OnEnter = function(self, slc, ...)
		G.print("sc_cube:OnEnter")
		slc.objlist = scbase.Init(self:getUpdGroup(), self:getDrawGroup())
		local fc = G.FPSCameraU.New()
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()
		upd:addObj(fc)

		local hTex = System.glres:loadTexture("block.jpg", G.GLRes.MipState.MipmapLinear, nil)
		local cube = G.CubeObj.New(self:getDrawGroup(), 1.0, hTex)
		cube:setOffset(G.Vec3.New(0,0,2))
		cube:setPriority(0x1000)
		upd:addObj(cube)

		local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1.0, nil)
		local fbc = G.FBClear.New(0x000, clp)
		dg:addObj(fbc)
	end,
	OnUpdate = function(self, slc, ...)
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
