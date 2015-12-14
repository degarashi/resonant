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
	engine:clearScene()
	-- 深度バッファ範囲指定
	engine:setDepthRange(G.Vec2.New(0, 20));
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
			ds = engine:getDrawScene(0x1000)
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
			G.Vec3.New(-2,-4,0), true)
	addObj(1.0, texBlock, texBlockNml, G.PrimitiveObj.Type.Sphere, false, false,
			G.Vec3.New(2,-3,0), true)
	addObj(2.0, texBlock, texBlockNml, G.PrimitiveObj.Type.Cone, true, false,
			G.Vec3.New(-3,3,4), true)
	do
		local hTex = System.glres:loadTexture("light.png", G.GLRes.MipState.MipmapLinear, nil)
		hTex:setFilter(true, true)
		local lit = G.PointSprite3D.New(hTex, G.Vec3.New(0,0,0))
		lit:setScale(G.Vec3.New(0.4, 0.4, 0.4))
		lit:setDrawPriority(0x3000)
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
		self.bCube = false
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
		local lscale = {3,4,4}
		local lfreq = {1,1.5,2}
		self.litangle = self.litangle + G.Degree.New(lv)
		local angv = self.litangle:toRadian():get()
		local lp = G.Vec3.New(G.math.sin(angv*lfreq[1])*lscale[1],
								G.math.cos(angv*lfreq[2])*lscale[2],
								-G.math.sin(angv*lfreq[3])*lscale[3] + 2)
		self.litpos = lp

		-- スポットライトと点光源を切り替え
		if Global.cpp.actScene:isKeyPressed() then
			self.bCube = not self.bCube
			self:InitScene()
		end

		local engine = Global.engine
		-- 光源位置のセット
		self.lit:setOffset(self.litpos)
		engine:setLightPosition(self.litpos)
		engine:setLightDir((G.Vec3.New(0,0,2) - self.litpos):normalization())
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
