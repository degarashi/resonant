#include "../sys_uniform_value.hpp"
#include "screen.hpp"
#include "../glx.hpp"

namespace rs {
	namespace util {
		// --------------------- PostEffect ---------------------
		const rs::IdValue T_PostEffect = GLEffect::GlxId::GenTechId("PostEffect", "Default");
		PostEffect::PostEffect(rs::Priority dprio):
			_alpha(0.5f)
		{
			_dtag.priority = dprio;
		}
		void PostEffect::setAlpha(float a) {
			_alpha = a;
		}
		void PostEffect::onDraw(rs::GLEffect& e) const {
			e.setTechPassId(T_PostEffect);
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
		FBSwitch::FBSwitch(rs::Priority dprio, rs::HFb hFb):
			_hlFb(hFb)
		{
			_dtag.priority = dprio;
		}
		void FBSwitch::onDraw(rs::GLEffect& e) const {
			if(_hlFb)
				e.setFramebuffer(_hlFb);
			else
				e.resetFramebuffer();
			// これ自体の描画はしない
		}

		// --------------------- FBClear ---------------------
		FBClear::FBClear(rs::Priority dprio,
						const rs::draw::ClearParam& p):
			_param(p)
		{
			_dtag.priority = dprio;
		}
		void FBClear::onDraw(rs::GLEffect& e) const {
			e.clearFramebuffer(_param);
		}
	}
}
