#pragma once
#include "../util/screen.hpp"

class BlurEffect : public rs::util::PostEffect {
	public:
		BlurEffect(rs::Priority p);
		void setAlpha(float a);
		void setDiffuse(rs::HTex ht);
};
DEF_LUAIMPORT(BlurEffect)
