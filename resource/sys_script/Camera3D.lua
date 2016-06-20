require("sysfunc")
return {
	-- 実行時調整可能な値定義
	_variable = {
		-- 変数名
		fov = {
			-- 値をどのように適用するか
			-- self: ターゲットオブジェクトハンドル
			-- value: セットする値 (number or Vec[2-4])
			apply = function(self, value)
				self:setFov(Degree.New(value))
			end,
			-- 値操作の方式
			manip = "linear",
			-- 一回の操作で加える量
			step = 1,
			-- デフォルト値
			value = 60
		},
		nearz = {
			apply = function(self, value)
				self:setNearZ(value)
			end,
			manip = "linear",
			step = 1,
			value = 1
		},
		farz = {
			-- 文字列なら同名のcpp関数をそのまま呼ぶ仕様
			apply = "setFarZ",
			manip = "exp",
			step = 1,
			base = 2,
			value = 100
		},
		offset = {
			apply = function(self, value)
				local pose = self:refPose()
				pose:setOffset(value)
			end,
			manip = "linear",
			step = 1,
			value = {1,0,1}
		}
	},
	_renamefunc =
		RS.RenameFunc(
			{"setPose", "setPose<spn::Pose3D>"},
			{"setFov", "setFov<spn::RadF>"},
			{"setAspect", "setAspect<float>"},
			{"setNearZ", "setNearZ<float>"},
			{"setFarZ", "setFarZ<float>"}
		)
}
