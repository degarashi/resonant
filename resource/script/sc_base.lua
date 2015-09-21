return {
	Init = function(hUpd, hDg)
		local ret = {}
		local pf = U_ProfileShow.New(hDg, 0, 0x1000)
		local inf = InfoShow.New(hDg, 0x2000)
		hUpd:addObj(inf)
		ret[#ret+1] = {hUpd, inf}
		hUpd:addObj(pf)
		ret[#ret+1] = {hUpd, pf}
		pf:setOffset(Vec2.New(0,480))
		hDg:setSortAlgorithmId({DrawGroup.SortAlg.Priority_Asc}, false)
		return ret
	end,
	CheckSwitch = function()
		local cp = Global.cpp
		local sc = System.scene
		if cp.actCube:isKeyPressed() then
			sc:setPushScene(sc_cube.New(), false)
		elseif cp.actSound:isKeyPressed() then
			sc:setPushScene(sc_sound.New(), false)
		elseif cp.actSprite:isKeyPressed() then
			sc:setPushScene(sc_sprite.New(), false)
		elseif cp.actSpriteD:isKeyPressed() then
			sc:setPushScene(sc_sprited.New(), false)
		elseif cp.actQuit:isKeyPressed() then
			sc:setPopScene(1)
		end
	end,
	Terminate = function(objlist)
		for i=1,#objlist do
			local p = objlist[i]
			p[1]:remObj(p[2])
		end
	end
}
