#pragma once
#include "../handle.hpp"
#include "../glx_id.hpp"
#include "spinner/pose.hpp"

namespace rs {
	struct DrawTag;
}
class Cube : public spn::Pose3D {
	private:
		static rs::WVb s_wVb;
		rs::HLVb		_hlVb;
		rs::HLTex		_hlTex;
		void _initVb();
	public:
		const static rs::IdValue	T_Cube,
									U_litpos;
		Cube(float s, rs::HTex hTex);
		void draw(Engine& e) const;
		void exportDrawTag(rs::DrawTag& d) const;
};
