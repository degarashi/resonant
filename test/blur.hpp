#pragma once
#include "dwrapper.hpp"
#include "screenrect.hpp"

class Blur : public rs::DrawableObjT<Blur> {
	private:
		using base_t = rs::DrawableObjT<Blur>;
		rs::HLTex	_hlTex;
		float		_alpha;
		ScreenRect	_rect;
	public:
		Blur(rs::Priority dprio);
		void setAlpha(float a);
		void setTexture(rs::HTex hTex);
		void onDraw(rs::GLEffect& e) const override;
};
class FBSwitch : public rs::DrawableObjT<FBSwitch> {
	private:
		rs::HLFb		_hlFb;
	public:
		FBSwitch(rs::Priority dprio, rs::HFb hFb);
		void onDraw(rs::GLEffect& e) const override;
};
class FBClear : public rs::DrawableObjT<FBClear> {
	private:
		rs::draw::ClearParam _param;
	public:
		FBClear(rs::Priority dprio,
				const rs::draw::ClearParam& p);
		void onDraw(rs::GLEffect& e) const override;
};
