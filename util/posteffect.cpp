#include "../sys_uniform_value.hpp"
#include "screen.hpp"
#include "../glx_if.hpp"

namespace rs {
	namespace util {
		// --------------------- PostEffect ---------------------
		PostEffect::PostEffect(IdValue idTech, rs::Priority dprio):
			_idTech(idTech)
		{
			_dtag.priority = dprio;
		}
		void PostEffect::_applyParam(IEffect& e) const {
			for(auto& p : _param)
				p.second(e);
		}
		void PostEffect::setParamFunc(IdValue id, const ParamF& f) {
			auto itr = std::find_if(_param.begin(), _param.end(), [id](auto& p){
				return p.first == id;
			});
			if(itr == _param.end())
				_param.emplace_back(id, f);
			else
				*itr = std::make_pair(id, f);
		}
		void PostEffect::clearParam() {
			_param.clear();
		}
		void PostEffect::onDraw(IEffect& e) const {
			e.setTechPassId(_idTech);
			_applyParam(e);
			// 重ねて描画
			_rect.draw(e);
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
