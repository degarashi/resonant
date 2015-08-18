#include "../sys_uniform_value.hpp"
#include "screen.hpp"
#include "../glx_if.hpp"

namespace rs {
	namespace util {
		// --------------------- PostEffect ---------------------
		PostEffect::PostEffect(IdValue idTech, rs::Priority dprio):
			_idTech(idTech),
			_alpha(0.5f)
		{
			_dtag.priority = dprio;
		}
		void PostEffect::setAlpha(float a) {
			_alpha = a;
		}
		void PostEffect::onDraw(IEffect& e) const {
			e.setTechPassId(_idTech);
			for(auto& p : _texture)
				e.setUniform(p.first, p.second);
			e.setUniform(unif::Alpha, _alpha);
			// 重ねて描画
			_rect.draw(e);
		}
		void PostEffect::setTexture(IdValue id, HTex hTex) {
			auto itr = std::find_if(_texture.begin(), _texture.end(), [id](auto& p){
				return p.first == id;
			});
			if(itr == _texture.end())
				_texture.emplace_back(id, hTex);
			else
				*itr = std::make_pair(id, hTex);
		}

		// --------------------- FBSwitch ---------------------
		FBSwitch::FBSwitch(rs::Priority dprio, rs::HFb hFb, const ClearParam_OP& cp):
			_hlFb(hFb),
			_cparam(cp)
		{
			_dtag.priority = dprio;
		}
		void FBSwitch::setClearParam(const ClearParam_OP& p) {
			_cparam = p;
		}
		// これ自体の描画はしない
		void FBSwitch::onDraw(IEffect& e) const {
			if(_hlFb)
				e.setFramebuffer(_hlFb);
			else
				e.resetFramebuffer();
			if(_cparam)
				e.clearFramebuffer(*_cparam);
		}

		// --------------------- FBClear ---------------------
		FBClear::FBClear(rs::Priority dprio,
						const rs::draw::ClearParam& p):
			_param(p)
		{
			_dtag.priority = dprio;
		}
		void FBClear::onDraw(IEffect& e) const {
			e.clearFramebuffer(_param);
		}
	}
}
