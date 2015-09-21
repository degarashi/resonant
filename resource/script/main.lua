function InitInput()
	Global.cpp.actQuit = System.input:makeAction("quit")
	Global.cpp.actReset = System.input:makeAction("reset")
	Global.cpp.actCube = System.input:makeAction("mode_cube")
	Global.cpp.actSound = System.input:makeAction("mode_sound")
	Global.cpp.actSprite = System.input:makeAction("mode_sprite")
	Global.cpp.actSpriteD = System.input:makeAction("mode_spriteD")
	Global.cpp.actAx = System.input:makeAction("axisX")
	Global.cpp.actAy = System.input:makeAction("axisY")
	Global.cpp.actMoveX = System.input:makeAction("moveX")
	Global.cpp.actMoveY = System.input:makeAction("moveY")
	Global.cpp.actPress = System.input:makeAction("press")
	local hK = Keyboard.OpenKeyboard(0)
	local hM = Mouse.OpenMouse(0)
	Global.cpp.hlIm = hM
	Global.cpp.hlIk = hK
	local code = {Action.Key._1, Action.Key._2, Action.Key._3, Action.Key._4, Action.Key._5}
	for i=1, #code do
		local st = "actNumber" .. (i-1)
		Global.cpp[st] = System.input:makeAction("number" .. (i-1))
		Global.cpp[st]:addLink(hK, Action.InputFlag.Button, code[i])
	end
	-- quit[Esc]		アプリケーション終了
	Global.cpp.actQuit:addLink(hK, Action.InputFlag.Button, Action.Key.Escape)
	-- reset-scene[R]	シーンリセット
	Global.cpp.actReset:addLink(hK, Action.InputFlag.Button, Action.Key.Lshift)
	-- mode: cube[Z]
	Global.cpp.actCube:addLink(hK, Action.InputFlag.Button, Action.Key.Z)
	-- mode: sound[X]
	Global.cpp.actSound:addLink(hK, Action.InputFlag.Button, Action.Key.X)
	-- mode: sprite[C]
	Global.cpp.actSprite:addLink(hK, Action.InputFlag.Button, Action.Key.C)
	-- mode: spriteD[V]
	Global.cpp.actSpriteD:addLink(hK, Action.InputFlag.Button, Action.Key.V)
	-- left, right, up, down [A,D,W,S]		カメラ移動
	Global.cpp.actAx:linkButtonAsAxis(hK, Action.Key.A, Action.Key.D)
	Global.cpp.actAy:linkButtonAsAxis(hK, Action.Key.S, Action.Key.W)
	-- rotate-camera[MouseX,Y]				カメラ向き変更
	Global.cpp.actMoveX:addLink(hM, Action.InputFlag.Axis, 0)
	Global.cpp.actMoveY:addLink(hM, Action.InputFlag.Axis, 1)
	Global.cpp.actPress:addLink(hM, Action.InputFlag.Button, 0)
end
function Initialize()
	System.lsys:loadClass("FPSCameraU")
	System.lsys:loadClass("sc_cube")
	System.lsys:loadClass("sc_sound")
	System.lsys:loadClass("sc_sprite")
	System.lsys:loadClass("sc_sprited")
	InitInput()
	-- ここでSceneハンドルを返すとそれが初期Sceneとなり、
	-- nilを返すとC++での指定となる
	return sc_cube.New()
end
