#pragma once
// #include "../updater.hpp"
// 視点を中心としたスカイドーム
// class SkyDome : public rs::DrawableObjT<SkyDome> {
// 	private:
// 		float		_height,	// 高さ
// 					_width;		// 横幅
// 		rs::HLVb	_vb;
// 		rs::HLIb	_ib;
// 		struct St_Default;
// 	public:
// 		SkyDome(int nH, int nV);
// 		void setHeight(float h);
// 		void setWidth(float w);
// };

#include "../util/screen.hpp"
class SkyDome : public rs::util::PostEffect {
	public:
		SkyDome(rs::Priority p);
		void setRayleighCoeff(const spn::Vec3& c);
		void setMieCoeff(float gain, float c);
		void setSunColor(const spn::Vec3& c);
		void setSunDir(const spn::Vec3& dir);
		void setSize(float w, float h);
		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(SkyDome)
