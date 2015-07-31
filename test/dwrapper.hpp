#pragma once
#include "../updater.hpp"
#include "../gameloophelper.hpp"
#include "engine.hpp"

template <class Base>
class DWrapper : public rs::DrawableObjT<DWrapper<Base>>,
				public Base,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		using base_dt = rs::DrawableObjT<DWrapper<Base>>;
		rs::IdValue		_tpId;
		rs::WDGroup		_wDGroup;
		rs::HDGroup _getDGroup() const {
			if(auto dg = _wDGroup.lock())
				return dg;
			return mgr_scene.getSceneBase().getDraw();
		}
	protected:
		struct St_Default : base_dt::template StateT<St_Default> {
			void onConnected(DWrapper& self, rs::HGroup hGroup) override {
				auto hl = self.handleFromThis();
				self._getDGroup()->get()->addObj(hl);
			}
			void onDisconnected(DWrapper& self, rs::HGroup hGroup) override {
				auto hl = self.handleFromThis();
				self._getDGroup()->get()->remObj(hl);
			}
			void onUpdate(DWrapper& self) override {
				self.refreshDrawTag();
			}
			void onDraw(const DWrapper& self, rs::GLEffect& e) const override {
				auto& fx = static_cast<Engine&>(e);
				fx.setTechPassId(self._tpId);
				self.Base::draw(fx);
			}
		};
	public:
		template <class... Ts>
		DWrapper(rs::IdValue tpId, rs::HDGroup hDg, Ts&&... ts):
			Base(std::forward<Ts>(ts)...),
			_tpId(tpId)
		{
			if(hDg)
				_wDGroup = hDg.weak();
			refreshDrawTag();
			base_dt::_dtag.idTechPass = tpId;
			base_dt::template setStateNew<St_Default>();
		}
		void setPriority(rs::Priority p) {
			base_dt::_dtag.priority = p;
		}
		void refreshDrawTag() {
			Base::exportDrawTag(base_dt::_dtag);
		}
};
