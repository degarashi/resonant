
		IClipSource_SP		_detailSource;
	public:
				const spn::PowInt	_samp_ratio;	//!< サンプリング倍率(256=等倍, 512=1つ飛ばし, 128=2倍アップサンプリング
				IClipSource_SP		_source;

// 				float getDrawScale() const;
// 				IOfs getDrawOffsetLocal(const spn::Vec2& center) const;
				static void Test(spn::PowSize s, spn::PowInt samp_ratio, rs::IEffect& e);
		};

		std::pair<int,int> _getTilePos() const;
		void _testDrawPolygon(Engine& e) const;

		void setDetailMap(const IClipSource_SP& src);
		void setDetailMapTexture(rs::HTex h);
};
