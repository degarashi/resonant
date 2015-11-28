#include "blureffect.hpp"
#include "../sys_uniform_value.hpp"
#include "../glx_if.hpp"

const rs::IdValue T_PostEffect = rs::IEffect::GlxId::GenTechId("PostEffect", "Default");
BlurEffect::BlurEffect(rs::Priority p): PostEffect(T_PostEffect, p) {}
void BlurEffect::setAlpha(float a) {
	setParam(rs::unif::Alpha, a);
}
void BlurEffect::setDiffuse(rs::HTex ht) {
	setParam(rs::unif::texture::Diffuse, ht);
}

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, BlurEffect, BlurEffect, "Object", NOTHING,
		(setAlpha)(setDiffuse), (rs::Priority))
