#pragma once
#include "../util/sharedgeom.hpp"
#include "../glx_id.hpp"
#include "../updater.hpp"
#include "spinner/pose.hpp"

extern const rs::IdValue
	T_ColFill,
	T_ColLine;

class ColBox : public rs::util::SharedGeometry<ColBox>, public spn::Pose3D {
	private:
		spn::Vec3	_color;
		float		_alpha;
	public:
		static rs::HLVb MakeVertex();
		static rs::HLIb MakeIndex();
		ColBox();
		void setAlpha(float a);
		void setColor(const spn::Vec3& c);
		void draw(rs::IEffect& e) const;
};
class ColBoxObj : public rs::DrawableObjT<ColBoxObj>, public ColBox {
	private:
		struct St_Default;
	public:
		ColBoxObj();
};
DEF_LUAIMPORT(ColBoxObj)
