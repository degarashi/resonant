function Test()
	local obj = myobj.New()
	-- 	同期読み込みテスト
	do
		local ret = System:loadResource("file:///tmp/test.jpg")
		RS.PrintValue("sync:", ret)
	end
	local tbl = {
		AAA = "file:///tmp/test.png",
		BBB = "file:///tmp/test.jpg",
		CCC = "file:///tmp/test.png"
	}
	do
		local retT = System:loadResources(tbl)
		RS.PrintValue("sync:", retT)
	end
	-- 非同期読み込みテスト
	do
		local id = System:loadResourcesASync(tbl)
		local num = 0
		repeat
			num = System:queryProgress(id)
			print(num)
		until num >= 1
		RS.PrintValue("async:", System:getResult(id))
	end
end
