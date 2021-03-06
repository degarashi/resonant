#pragma once
#include "spinner/pose.hpp"
#include "../handle.hpp"
#include "../glx_id.hpp"

namespace rs {
	struct DrawTag;
}
class Engine;
//! 常にカメラと正対するスプライト
class Sprite3D : public spn::Pose3D {
	private:
		rs::HLTex	_hlTex;
		rs::HLVb	_hlVb;
		rs::HLIb	_hlIb;
		float		_alpha;
	public:
		const static rs::IdValue	T_Sprite;
		Sprite3D(rs::HTex hTex);
		void draw(Engine& e) const;
		void setAlpha(float a);
		void exportDrawTag(rs::DrawTag& d) const;
};
#include "../util/dwrapper.hpp"
class PointSprite3D : public rs::DrawableObjT<PointSprite3D>,
						public Sprite3D
{
	private:
		struct St_Default;
	public:
		PointSprite3D(rs::HTex hTex, const spn::Vec3& pos);
};
