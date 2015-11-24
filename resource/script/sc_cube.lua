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

		do
			local hTex = System.glres:loadTexture("block.jpg", G.GLRes.MipState.MipmapLinear, nil)
			hTex:setFilter(true, true)
			local cube = G.CubeObj.New(self:getDrawGroup(), 1.0, hTex, false)
			cube:setOffset(G.Vec3.New(0,0,2))
			cube:setPriority(0x1000)
			upd:addObj(cube)

			self.cube = cube
		end
		do
			local hTex = System.glres:loadTexture("floor.jpg", G.GLRes.MipState.MipmapLinear, nil)
			hTex:setFilter(true, true)
			local room = G.CubeObj.New(self:getDrawGroup(), 6.0, hTex, true)
			room:setOffset(G.Vec3.New(0,0,2))
			room:setPriority(0x1000)
			upd:addObj(room)

			self.room = room
		end
		do
			local hTex = System.glres:loadTexture("light.png", G.GLRes.MipState.MipmapLinear, nil)
			hTex:setFilter(true, true)
			local lit = G.PointSprite3D.New(self:getDrawGroup(), hTex, G.Vec3.New(0,0,0))
			lit:setPriority(0x2000)
			lit:setScale(G.Vec3.New(0.4, 0.4, 0.4))
			upd:addObj(lit)
			self.lit = lit
		end
		local clp = G.ClearParam.New(G.Vec4.New(0,0,0,0), 1.0, nil)
		local fbc = G.FBClear.New(0x000, clp)
		dg:addObj(fbc)

		self.litpos = G.Vec3.New(0,0,0)
		self.litangle = G.Degree.New(0)
	end,
	OnUpdate = function(self, slc, ...)
		-- ボタン操作で光源を回転
		local lv = 0
		if Global.cpp.actLightR0:isKeyPressing() then
			lv = 1
		elseif Global.cpp.actLightR1:isKeyPressing() then
			lv = -1
		end
		self.litangle = self.litangle + G.Degree.New(lv)
		local angv = self.litangle:toRadian():get()
		local lp = G.Vec3.New(G.math.sin(angv)*3,
								G.math.cos(angv)*3,
								-G.math.sin(angv)*3 + 2)
		self.litpos = lp

		-- 光源位置のセット
		Global.engine:setLightPosition(self.litpos)
		self.lit:setOffset(self.litpos)
		-- キューブを回転
		self.cube:advance()
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
