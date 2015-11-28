local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end
function InitScene(self)
	local upd,dg = self:getUpdGroup(), self:getDrawGroup()
	local engine = Global.engine
	-- 深度バッファ範囲指定
	engine:setDepthRange(G.Vec2.New(0, 50));
	-- ライト深度バッファサイズ指定
	engine:setLightDepthSize({512,512})
	do
		if self.prevDS then
			dg:remObj(self.prevDS)
		end
		local ds = engine:getDrawScene(0x1000)
		dg:addObj(ds)
		self.prevDS = ds
	end
	do
		local hTex = System.glres:loadTexture("block.jpg", G.GLRes.MipState.MipmapLinear, nil)
		hTex:setFilter(true, true)
		local cube = G.CubeObj.New(1.0, hTex, false)
		cube:setOffset(G.Vec3.New(0,0,2))
		engine:addSceneObject(cube)
		self.cube = cube
	end
	do
		local hTex = System.glres:loadTexture("floor.jpg", G.GLRes.MipState.MipmapLinear, nil)
		hTex:setFilter(true, true)
		local room = G.CubeObj.New(6.0, hTex, true)
		room:setOffset(G.Vec3.New(0,0,2))
		engine:addSceneObject(room)
		self.room = room
	end
	do
		local hTex = System.glres:loadTexture("light.png", G.GLRes.MipState.MipmapLinear, nil)
		hTex:setFilter(true, true)
		local lit = G.PointSprite3D.New(hTex, G.Vec3.New(0,0,0))
		lit:setScale(G.Vec3.New(0.4, 0.4, 0.4))
		engine:addSceneObject(lit)
		self.lit = lit
	end
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
		local clp = G.ClearParam.New(G.Vec4.New(0,1,0,0), 1.0, nil)
		local fbc = G.FBClear.New(0x000, clp)
		dg:addObj(fbc)

		self:InitScene()
		self.litpos = G.Vec3.New(0,0,0)
		self.litangle = G.Degree.New(0)
	end,
	OnEffectReset = function(self, slc, ...)
		self:InitScene()
	end,
	OnUpdate = function(self, slc, ...)
		-- ボタン操作で光源を回転
		local lv = 0
		if Global.cpp.actLightR0:isKeyPressing() then
			lv = 1
		elseif Global.cpp.actLightR1:isKeyPressing() then
			lv = -1
		end
		local lscale = 4
		self.litangle = self.litangle + G.Degree.New(lv)
		local angv = self.litangle:toRadian():get()
		local lp = G.Vec3.New(G.math.sin(angv)*lscale,
								G.math.cos(angv)*lscale,
								-G.math.sin(angv)*lscale + 2)
		self.litpos = lp

		local engine = Global.engine
		-- 光源位置のセット
		self.lit:setOffset(self.litpos)
		engine:setLightPosition(self.litpos)
		engine:setLightDir((G.Vec3.New(0,0,2) - self.litpos):normalization())
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
