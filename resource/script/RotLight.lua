BaseClass = "U_ObjectUpd"
InitialState = "st_idle"

-- \param[in] origin[vec3]
-- \param[in] dirorigin[vec3]
-- \param[in] radius[vec3]
-- \param[in] freq[vec3]
-- \param[in] angle[degree]
-- \param[in] color[vec3]
function Ctor(self, origin, dirorigin, radius, freq, angle, color, dg)
	G.print("1")
	self.origin = origin
	self.dirorigin = dirorigin
	self.radius = radius
	self.freq = freq
	self.angle = angle
	self.color = color
	self.dg = dg
	self._base.Ctor(self, InitialState)
end

st_idle = {
	OnEnter = function(self, slc, ...)
		local engine = Global.engine
		self.litId = engine:makeLight()
		local lit = engine:getLight(self.litId)
		-- 深度バッファ範囲指定
		lit:setDepthRange(G.Vec2.New(0.01, 100))
		lit:setColor(self.color)
		do
			local hTex = System.glres:loadTexture("light.png", G.GLRes.MipState.MipmapLinear, nil)
			hTex:setFilter(true, true)
			local lit = G.PointSprite3D.New(hTex, G.Vec3.New(0,0,0))
			lit:setScale(G.Vec3.New(0.4, 0.4, 0.4))
			lit:setDrawPriority(0x3000)
			self.dg:addObj(lit)
			self.litSpr = lit
		end
	end,
	OnUpdate = function(self, slc, ...)
		self:RecvMsg("Advance", G.Degree.New(1))
		local angv = self.angle:toRadian():get()
		local freq = self.freq
		local rad = self.radius
		local lp = G.Vec3.New(G.math.sin(angv*freq.x)*rad.x,
								G.math.cos(angv*freq.y)*rad.y,
								-G.math.sin(angv*freq.z+G.math.pi/2)*rad.z)
		local litpos = lp + self.origin
		-- 光源位置のセット
		self.litSpr:setOffset(litpos)
		local engine = Global.engine
		local lit = engine:getLight(self.litId)
		lit:setPosition(litpos)
		lit:setDirection((self.dirorigin - litpos):normalization())
	end,
	OnExit = function(self, slc, ...)
	end,
	Advance = function(self, slc, diff)
		self.angle = self.angle + diff
	end
}
