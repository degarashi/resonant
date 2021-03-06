function InitTweak()
	local tk = {
		x = System.input:makeAction("tweak_x"),
		y = System.input:makeAction("tweak_y"),
		z = System.input:makeAction("tweak_z"),
		w = System.input:makeAction("tweak_w"),
		cx = System.input:makeAction("tweak_cx"),
		cy = System.input:makeAction("tweak_cy"),
		sw = System.input:makeAction("tweak_sw"),
		cont = System.input:makeAction("tweak_cont"),
		save = System.input:makeAction("tweak_save")
	}
	local hK = Keyboard.OpenKeyboard(0)
	tk.x:linkButtonAsAxis(hK, Action.Key.J, Action.Key.U)
	tk.y:linkButtonAsAxis(hK, Action.Key.K, Action.Key.I)
	tk.z:linkButtonAsAxis(hK, Action.Key.L, Action.Key.O)
	tk.w:linkButtonAsAxis(hK, Action.Key.Comma, Action.Key.Kp_at)
	tk.cx:linkButtonAsAxis(hK, Action.Key.J, Action.Key.L)
	tk.cy:linkButtonAsAxis(hK, Action.Key.K, Action.Key.I)
	tk.sw:addLink(hK, Action.InputFlag.Button, Action.Key.Tab)
	tk.cont:addLink(hK, Action.InputFlag.Button, Action.Key.Lshift)
	tk.save:addLink(hK, Action.InputFlag.Button, Action.Key.Return)
	Global.tweak = tk
end
function InitInput()
	local g = Global.cpp
	Global.cpp.actQuit = System.input:makeAction("quit")
	Global.cpp.actReset = System.input:makeAction("reset")
	Global.cpp.actCube = System.input:makeAction("mode_cube")
	Global.cpp.actSound = System.input:makeAction("mode_sound")
	Global.cpp.actSprite = System.input:makeAction("mode_sprite")
	Global.cpp.actSpriteD = System.input:makeAction("mode_spriteD")
	Global.cpp.actRayleigh = System.input:makeAction("mode_rayleigh")
	Global.cpp.actClipmap = System.input:makeAction("mode_clipmap")
	Global.cpp.actAx = System.input:makeAction("axisX")
	Global.cpp.actAy = System.input:makeAction("axisY")
	Global.cpp.actMoveX = System.input:makeAction("moveX")
	Global.cpp.actMoveY = System.input:makeAction("moveY")
	Global.cpp.actPress = System.input:makeAction("press")
	Global.cpp.actLightR0 = System.input:makeAction("light0")
	Global.cpp.actLightR1 = System.input:makeAction("light1")
	Global.cpp.actScene = System.input:makeAction("sw_scene")
	g.actFilterUpB = System.input:makeAction("bfilter_up")
	g.actFilterDownB = System.input:makeAction("bfilter_down")
	g.actFilterUpG = System.input:makeAction("gfilter_up")
	g.actFilterDownG = System.input:makeAction("gfilter_down")
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
	-- mode: rayleigh[B]
	Global.cpp.actRayleigh:addLink(hK, Action.InputFlag.Button, Action.Key.B)
	-- mode: clipmap[N]
	Global.cpp.actClipmap:addLink(hK, Action.InputFlag.Button, Action.Key.N)
	-- left, right, up, down [A,D,W,S]		カメラ移動
	Global.cpp.actAx:linkButtonAsAxis(hK, Action.Key.A, Action.Key.D)
	Global.cpp.actAy:linkButtonAsAxis(hK, Action.Key.S, Action.Key.W)
	-- [Q, E] ライト回転
	Global.cpp.actLightR0:addLink(hK, Action.InputFlag.Button, Action.Key.Q)
	Global.cpp.actLightR1:addLink(hK, Action.InputFlag.Button, Action.Key.E)
	-- rotate-camera[MouseX,Y]				カメラ向き変更
	Global.cpp.actMoveX:addLink(hM, Action.InputFlag.Axis, 0)
	Global.cpp.actMoveY:addLink(hM, Action.InputFlag.Axis, 1)
	Global.cpp.actPress:addLink(hM, Action.InputFlag.Button, 0)
	-- [J] シーン切り替え
	Global.cpp.actScene:addLink(hK, Action.InputFlag.Button, Action.Key.J)
	-- [I,K,O,L] フィルタ係数
	-- g.actFilterUpB:addLink(hK, Action.InputFlag.Button, Action.Key.I)
	-- g.actFilterDownB:addLink(hK, Action.InputFlag.Button, Action.Key.K)
	-- g.actFilterUpG:addLink(hK, Action.InputFlag.Button, Action.Key.O)
	-- g.actFilterDownG:addLink(hK, Action.InputFlag.Button, Action.Key.L)
end
function Initialize()
	System.lsys:loadClass("FPSCameraU")
	System.lsys:loadClass("sc_cube")
	System.lsys:loadClass("sc_sound")
	System.lsys:loadClass("sc_sprite")
	System.lsys:loadClass("sc_sprited")
	System.lsys:loadClass("sc_rayleigh")
	System.lsys:loadClass("sc_clipmap")
	System.lsys:loadClass("RotLight")
	InitInput()
	InitTweak()
	-- ここでSceneハンドルを返すとそれが初期Sceneとなり、
	-- nilを返すとC++での指定となる
	return sc_clipmap.New()
end
