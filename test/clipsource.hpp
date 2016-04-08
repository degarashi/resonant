#pragma once
#include "../glresource.hpp"
#include "cliphash.hpp"

struct IClipSource {
	struct Data {
		rs::HLTex	tex;
		spn::Vec4	uvrect;
		spn::Vec2	unit;

		Data(rs::HTex t, const spn::RectF& r, const spn::SizeF& s);
		Data(rs::HTex t, const spn::Rect& r, const spn::Size& s);
	};
	//! 高低差レンジ
	virtual spn::RangeF getRange() const = 0;
	//! 指定された矩形のデータをテクスチャとして取得
	/*! 次回の呼び出しで返されたテクスチャの内容が変更される場合がある */
	// type=(R16 texture) 512 levels
	virtual Data getDataRect(const spn::Rect& r) = 0;
};
using IClipSource_SP = std::shared_ptr<IClipSource>;
DEF_LUAIMPORT(IClipSource_SP)
DEF_LUAIMPORT(Hash_SP)
DEF_LUAIMPORT(HashVec_SP)
DEF_LUAIMPORT(IHash)
DEF_LUAIMPORT(IHashVec)

class ClipPNSource : public IClipSource {
	private:
		HashVec_SP			_hash;
		rs::HLTex			_hTex;
		const int			_freq;
	public:
		ClipPNSource(const HashVec_SP& sp,
					spn::PowInt tsize,
					int freq);
		static IClipSource_SP Create(const HashVec_SP& sp,
									int tsize,
									int freq);

		spn::RangeF getRange() const override;
		Data getDataRect(const spn::Rect& r) override;
};
class ClipTexSource : public IClipSource {
	private:
		rs::HLTex	_texture;
	public:
		ClipTexSource(rs::HTex t);
		static IClipSource_SP Create(rs::HTex t);

		spn::RangeF getRange() const override;
		Data getDataRect(const spn::Rect& r) override;
};
class ClipTestSource : public IClipSource {
	private:
		const float		_freqH,
						_freqV;
		rs::HLTex		_hTex;
		const float		_aux;
	public:
		ClipTestSource(float fH, float fV, spn::PowSize size, float aux);

		void save(const std::string& path) const;
		spn::RangeF getRange() const override;
		Data getDataRect(const spn::Rect& r) override;
		static float TestElev(int w, float ratio, float x, float y);
};
