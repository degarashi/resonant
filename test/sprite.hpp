#pragma once
#include "spinner/pose.hpp"
#include "../handle.hpp"
#include "spinner/structure/range.hpp"
#include "../glx_id.hpp"

namespace rs {
	struct DrawTag;
}
class Engine;
//! 表示テスト用のスプライト
class Sprite : public spn::Pose2D {
	private:
		static rs::WVb	s_wVb;
		static rs::WIb	s_wIb;
		rs::HLVb	_hlVb;
		rs::HLIb	_hlIb;
		rs::HLTex	_hlTex;
		spn::RangeF	_zRange;
		float		_zOffset,
					_alpha;
	public:
		static std::pair<rs::HLVb, rs::HLIb> InitBuffer();
		const static rs::IdValue	T_Sprite;
		Sprite(rs::HTex hTex, float z);
		void draw(Engine& e) const;
		void setZOffset(float z);
		void setZRange(const spn::RangeF& r);
		void setAlpha(float a);
		void exportDrawTag(rs::DrawTag& d) const;
};
#include "../util/dwrapper.hpp"
class BoundingSprite : public rs::DrawableObjT<BoundingSprite>,
						public Sprite
{
	private:
		spn::Vec2	_svec;
		struct St_Default;
	public:
		BoundingSprite(rs::HTex hTex, const spn::Vec2& pos, const spn::Vec2& svec);
};
DEF_LUAIMPORT(BoundingSprite)
class SpriteObj : public rs::DrawableObjT<SpriteObj>,
					public Sprite
{
	private:
		struct St_Default;
	public:
		SpriteObj(rs::HTex hTex, float depth);
};
DEF_LUAIMPORT(SpriteObj)
