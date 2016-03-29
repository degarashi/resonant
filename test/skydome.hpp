#pragma once
#include "../util/screen.hpp"
class SkyDome : public rs::util::PostEffect {
	public:
		SkyDome(rs::Priority p);
		//! 仮想スカイドームの距離計算で使用(X=大気の横幅反比, Y=大気の厚さ)
		void setScale(float w, float h);
		//! 仮想スカイドームの補正比率
		void setDivide(float d);
		//! レイリー散乱係数
		void setRayleighCoeff(const spn::Vec3& r);
		//! ミー散乱係数
		void setMieCoeff(float gain, float c);
		//! ミー散乱の入射角度に対する係数
		void setLightPower(float p);
		//! 全体の光量補正値
		void setLightDir(const spn::Vec3& dir);
		//! 太陽の方向
		void setLightColor(const spn::Vec3& c);

		void onDraw(rs::IEffect& e) const override;
};
DEF_LUAIMPORT(SkyDome)
