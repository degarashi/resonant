-- システム関数はRSに纏める
RS = {}

local print = print
local rawget, rawset = rawget, rawset
local pairs = pairs
local type = type
local tostring = tostring
--! デバッグ用途の変数のプリント。テーブルの深度対応
--[[!
	\param name[in](any) エントリ名
	\param val[in](any) 表示する変数
	\param depth[in](number) 表示深度 = 0
	\param indent[in](number) インデント幅 = 0
]]
function RS.PrintValue(name, val, depth, mem)
	depth = depth or 3
	mem = mem or {
		indent = 0,
		map = {},
		id = 0
	}
	local indentS = ""
	for i=1,mem.indent do
		indentS = indentS .. "\t"
	end
	local keyS = indentS .. "[" .. tostring(name) .. "] = "
	if type(val) == "table" then
		if mem.map[val] then
			-- 既に表示済みなので通し番号だけ表示する
			print(keyS .. "(table)id=" .. mem.map[val])
		else
			mem.map[val] = mem.id
			mem.id = mem.id + 1
			if depth == 0 then
				-- エントリを1つでも持っているかチェック
				local bEntry = false
				for k,v in pairs(val) do
					bEntry = true
					break
				end
				if bEntry then
					keyS = keyS .. "{...}"
				end
				print(keyS)
			else
				mem.indent = mem.indent + 1
				depth = depth - 1
				print(keyS .. "id=" .. mem.map[val] .. "{")
				for k,v in pairs(val) do
					RS.PrintValue(k, v, depth, mem)
				end
				print(indentS .. "]")
				mem.indent = mem.indent - 1
			end
		end
	else
		print(keyS .. tostring(val))
	end
end
function RS.Concat(src0, src1)
	local n = #src0
	for i=1,#src1 do
		n = n+1
		src0[n] = src1[i]
	end
end
function RS.Join(dst, src)
	for k,v in pairs(src) do
		local v = dst[k]
		if v == nil then
			dst[k] = v
		end
	end
end
function RS.JoinRaw(dst, src)
	for k,v in pairs(src) do
		local v = rawget(dst, k)
		if v == nil then
			rawset(dst, k, v)
		end
	end
end
function RS.Overwrite(dst, src)
	for k,v in pairs(src) do
		dst[k] = v
	end
end
function RS.OverwriteRaw(dst, src)
	for k,v in pairs(src) do
		rawset(dst, k, v)
	end
end
function RS.CallOperator(a, b, op)
	local t,postfix = type(b)
	if t == "table" then
		postfix = b._postfix
	else
		assert(t == "number")
		postfix = "F"
	end
	return a[op .. postfix](a,b)
end
function RS.RenameFuncJoin(tbl, entry, ...)
	if not entry then
		return
	end
	tbl[entry[1]] = function(self, ...)
		return self[entry[2]](self, ...)
	end
	RS.RenameFuncJoin(tbl, ...)
end
function RS.RenameFunc(entry, ...)
	if not entry then
		return
	end
	local tbl = {}
	RS.RenameFuncJoin(tbl, entry, ...)
	return tbl
end
local G = _G
function RS.RenameFuncStatic(tbl, tblname, entry, ...)
	if not entry then
		return
	end
	tbl[entry[1]] = function(...)
		return G[tblname][entry[2]](...)
	end
	RS.RenameFuncStatic(tbl, tblname, ...)
end
