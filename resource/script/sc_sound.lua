local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)
end

st_idle = {
	OnEnter = function(self, slc, ...)
		G.print("sc_sound:OnEnter")
		local upd,dg = self:getUpdGroup(), self:getDrawGroup()
		slc.objlist = scbase.Init(upd, dg)

		local clp = G.ClearParam.New(G.Vec4.New(0,0,0.15,0), 1.0, nil)
		local fbc = G.FBClear.New(0x000, clp)
		dg:addObj(fbc)

		-- サウンド読み込み
		self.hAb = System.sound:loadOggStream("test_music.ogg")
		self.hSg = System.sound:createSourceGroup(1)
		self.hSg:play(self.hAb, 0)
	end,
	OnUpdate = function(self, slc, ...)
		scbase.CheckSwitch()
	end,
	OnExit = function(self, slc, ...)
		G.print("sc_sound:OnExit")
		self.hSg:clear()
	end,
	GetState = function()
		return "Sound"
	end
}
