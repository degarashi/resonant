#include "../sys_uniform_value.hpp"
#include "screen.hpp"
#include "../glx_if.hpp"
#include "../systeminfo.hpp"
#include "../sys_uniform.hpp"

namespace rs {
	namespace util {
		const IdValue PostEffect::U_RectScale = IEffect::GlxId::GenUnifId("u_rectScale");
		// --------------------- PostEffect ---------------------
		PostEffect::PostEffect(IdValue idTech, rs::Priority dprio):
			_idTech(idTech)
		{
			_dtag.priority = dprio;
			setRect({-1,1,-1,1});
		}
		void PostEffect::setTechPassId(IdValue idTech) {
			_idTech = idTech;
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
		void PostEffect::setRect(const spn::RectF& r) {
			_drawRect = r;
		}
		void PostEffect::onDraw(IEffect& e) const {
			e.setTechPassId(_idTech);
			_applyParam(e);
			e.setVDecl(DrawDecl<vdecl::screen>::GetVDecl());
			e.setVStream(_rect11.getVertex(), 0);
			auto ib = _rect11.getIndex();
			e.setIStream(ib);
			e.setUniform<false>(U_RectScale, spn::Vec4{
					(_drawRect.x0+_drawRect.x1)/2,
					(_drawRect.y0+_drawRect.y1)/2,
					_drawRect.width()/2,
					_drawRect.height()/2});
			// 重ねて描画
			e.drawIndexed(GL_TRIANGLES, ib->get()->getNElem(), 0);
		}

		// --------------------- Viewport ---------------------
		Viewport::Viewport(Priority dprio) {
			_dtag.priority = dprio;
			setByRatio({0,1,0,1});
		}
		void Viewport::setByRatio(const spn::RectF& r) {
			_bPixel = false;
			_rect = r;
		}
		void Viewport::setByPixel(const spn::RectF& r) {
			_bPixel = true;
			_rect = r;
		}
		void Viewport::onDraw(IEffect& e) const {
			e.setViewport(_bPixel, _rect);
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

			// ビューポートはフルスクリーンで初期化
			Viewport(0x0000).onDraw(e);
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
