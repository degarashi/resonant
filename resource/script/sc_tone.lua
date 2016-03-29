local scbase = G.require("sc_base")

BaseClass = "U_Scene"
InitialState = "st_idle"
function Ctor(self, ...)
	self.bCube = false
	self._base.Ctor(self, InitialState, ...)
end

st_idle = {
	OnEnter = function(self, slc, ...)
	end
}
