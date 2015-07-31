#pragma once
#include "spinner/pose.hpp"
#include "../handle.hpp"
#include "spinner/structure/range.hpp"

namespace rs {
	struct DrawTag;
}
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
		void _initBuffer();
	public:
		const static rs::IdValue	T_Sprite;
		Sprite(rs::HTex hTex, float z);
		void draw(Engine& e) const;
		void setZOffset(float z);
		void setZRange(const spn::RangeF& r);
		void setAlpha(float a);
		void exportDrawTag(rs::DrawTag& d) const;
};
