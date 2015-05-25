#pragma once
#include "spinner/pose.hpp"
#include "../handle.hpp"

class Engine;
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
		float		_zOffset;
		void _initBuffer();
	public:
		const static rs::IdValue	T_Sprite;
		Sprite(rs::HTex hTex, float z);
		void draw(Engine& e) const;
		void exportDrawTag(rs::DrawTag& d) const;
};

