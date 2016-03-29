#include "tonemap.hpp"
#include "../glresource.hpp"
#include "../glx_if.hpp"
#include "../sys_uniform_value.hpp"

namespace {
	using GId = rs::IEffect::GlxId;
	const rs::IdValue T_ToneAvg			= GId::GenTechId("Tone", "YAverage"),
					T_ToneShrink		= GId::GenTechId("Tone", "YShrink"),
					T_ToneApply			= GId::GenTechId("Tone", "Tone"),
					U_TexInfo			= GId::GenUnifId("u_texInfo"),
					U_MapWidth			= GId::GenUnifId("u_mapWidth");
	using Att = rs::GLFBuffer::Att;
}
spn::RFlagRet ToneMap::_refresh(spn::Size& ps, ScreenSizeP*) const {
	ps = getSource()->get()->getSize();
	ps.shiftR_2pow();
	return {true, 0};
}
spn::RFlagRet ToneMap::_refresh(rs::HLTex& h, Shrink0*) const {
	auto sz = getScreenSizeP();
	h = mgr_gl.createTexture(sz, GL_RG16F, false, false);
	_fbFirst->get()->attachTexture(Att::COLOR0, h);
	return {true, 0};
}
spn::RFlagRet ToneMap::_refresh(HLTexV& h, ShrinkA*) const {
	h.clear();
	auto sz = getScreenSizeP();
	for(;;) {
		// 1/8にする
		sz.shiftR_one(1);
		sz.shiftR_one(1);
		h.emplace_back(mgr_gl.createTexture(sz, GL_RG16F, false, false));
		if(sz.width == 1)
			break;
	}
	return {true, 0};
}
spn::RFlagRet ToneMap::_refresh(rs::HLTex& h, Result*) const {
	auto sz = getSource()->get()->getSize();
	h = mgr_gl.createTexture(sz, GL_RGB8, false, false);
	_fbApply->get()->attachTexture(Att::COLOR0, h);
	return {true, 0};
}

ToneMap::ToneMap(rs::Priority p) {
	base_t::setDrawPriority(p);
	auto fb = [](){ return mgr_gl.makeFBuffer(); };
	_fbFirst = fb();
	_fbShrink = fb();
	_fbApply = fb();
	setStateNew<St_Default>();
}
struct ToneMap::St_Default : StateT<St_Default> {
	void onDraw(const ToneMap& self, rs::IEffect& e) const override {
		auto fb0 = e.getFramebuffer();
		auto& tf = self.getShrink0();
		auto& src = self.getSource();
		e.setTechPassId(T_ToneAvg);
		e.setUniform(rs::unif::texture::Diffuse, src);
		auto srcsize = src->get()->getSize();
		e.setUniform(U_MapWidth, spn::Vec2(srcsize.width, srcsize.height));
		e.setFramebuffer(self._fbFirst);
		self._rect.draw(e);
// 		static int cou = 0;
// 		if(++cou == 300)
// 			tf->get()->save("/tmp/first.png");

		auto& ts = self.getShrinkA();
		const int nT = ts.size();
		for(int i=0 ; i<nT ; i++) {
			e.setTechPassId(T_ToneShrink);
			auto prev = (i==0) ? tf : ts[i-1];
			e.setUniform(rs::unif::texture::Diffuse, prev);
			auto tsize = prev->get()->getSize();
			e.setUniform(U_MapWidth, spn::Vec2(tsize.width, tsize.height));
			self._fbShrink->get()->attachTexture(Att::COLOR0, ts[i]);
			e.setFramebuffer(self._fbShrink);
			self._rect.draw(e);
// 			if(cou == 300)
// 				ts[i]->get()->save((boost::format("/tmp/layer%1%.png") % i).str());
		}
		// ts[ts.size()-1]->get()->use_begin();
		// GLfloat dst[2*4];
		// GL.glGetTexImage(ts[ts.size()-1]->get()->getFaceFlag(), 0, GL_RG, GL_FLOAT, dst);
		// std::cout << "dst.x=" << dst[0] << std::endl;
		// std::cout << "dst.y=" << dst[1] << std::endl;

		e.setTechPassId(T_ToneApply);
		e.setUniform(U_TexInfo, ts.back());
		e.setUniform(rs::unif::texture::Diffuse, src);
		e.setFramebuffer(self._fbApply);
		self._rect.draw(e);

		e.setFramebuffer(fb0);
	}
};
#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ToneMap, ToneMap, "DrawableObj", NOTHING,
		(setSource<rs::HTex>)
		(getResult)(getShrink0)(getShrinkA),
		(rs::Priority)
)
