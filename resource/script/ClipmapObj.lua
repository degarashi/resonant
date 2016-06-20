return {
	_variable = {
		gridsize = {
			apply = function(self, value)
				self:setGridSize(value.x, value.y)
			end,
			manip = "linear",
			step = 1,
			value = {1,1}
		},
		divide = {
			apply = function(self, value)
				self:setDivide(value)
			end,
			manip = "linear",
			step = 1e2,
			value = 4e3
		},
		rayleighC = {
			apply = function(self, value)
				self:setRayleighCoeff(value)
			end,
			manip = "linear",
			step = 1e-2,
			value = {680*1e-3, 550*1e-3, 450*1e-3}
		},
		mieC = {
			apply = function(self, value)
				self:setMieCoeff(value.x, value.y)
			end,
			manip = "linear",
			step = 1e-1,
			value = {0.8, 0.5}
		},
		litPower = {
			apply = function(self, value)
				self:setLightPower(value)
			end,
			manip = "linear",
			step = 1,
			value = 15.4
		},
		litDir = {
			apply = function(self, value)
				self:setLightDir(value)
			end,
			manip = "dir3d",
			step = 1,
			value = {0,-1,0}
		},
		litColor = {
			apply = function(self, value)
				self:setLightColor(value)
			end,
			manip = "linear",
			step = 1e-1,
			value = {1,1,1}
		}
	}
}
