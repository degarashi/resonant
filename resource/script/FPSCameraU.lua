-- C++でベースにするクラス
BaseClass = "FPSCamera"
-- 初期ステート設定
InitialState = "st_idle"
-- コンストラクタ
function Ctor(self, ...)
	self._base.Ctor(self, InitialState, ...)

	self.bPress = false
	self.yaw = G.Radian.New(0)
	self.pitch = G.Radian.New(0)
	self.roll = G.Radian.New(0)
end
st_idle = {
	OnEnter = function(self, slc, ...)
		G.print("FPS: OnEnter")
	end,
	OnUpdate = function(self, slc, ...)
		local btn = Global.cpp.actPress:isKeyPressing()
		if btn ~= self.bPress then
			local flag = self.bPress and G.Action.MouseMode.Absolute or G.Action.MouseMode.Relative
			G.print(flag)
			Global.cpp.hlIm:setMouseMode(flag)
			self.bPress = btn
		end
		local speed = 2.15
		local mvF = speed * Global.cpp.actAy:getKeyValueSimplified()
		local mvS = speed * Global.cpp.actAx:getKeyValueSimplified()
		local pose = Global.cpp.hlCam:refPose()
		pose:moveFwd3D(mvF)
		pose:moveSide3D(mvS)
		if self.bPress then
			local xv = Global.cpp.actMoveX:getValue()/6.0
			local yv = Global.cpp.actMoveY:getValue()/6.0
			self.yaw = self.yaw + G.Degree.New(xv)
			self.pitch = self.pitch + G.Degree.New(-yv)
			self.yaw:single()
			self.pitch:rangeValue(G.Degree.New(-89):toRadian():get(),
									G.Degree.New(89):toRadian():get())
			pose:setRot(G.Quat.RotationYPR(self.yaw, self.pitch, self.roll))
		end
-- 		G.print("FPS: OnUpdate")
	end
}
