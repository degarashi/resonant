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
	protected:
		struct St_Default : base_dt::template StateT<St_Default> {
			void onConnected(DWrapper& self, rs::HGroup hGroup) override {
				auto d = mgr_scene.getSceneBase().getDraw();
				auto hl = self.handleFromThis();
				d->get()->addObj(hl);
			}
			void onDisconnected(DWrapper& self, rs::HGroup hGroup) override {
				auto d = mgr_scene.getSceneBase().getDraw();
				auto hl = self.handleFromThis();
				d->get()->remObj(hl);
			}
			void onDraw(const DWrapper& self, rs::GLEffect& e) const override {
				auto& fx = static_cast<Engine&>(e);
				fx.setTechPassId(self._tpId);
				self.draw(fx);
			}
		};
	public:
		template <class... Ts>
		DWrapper(rs::IdValue tpId, Ts&&... ts):
			Base(std::forward<Ts>(ts)...),
			_tpId(tpId)
		{
			refreshDrawTag();
			base_dt::_dtag.idTechPass = tpId;
			base_dt::template setStateNew<St_Default>();
		}
		void refreshDrawTag() {
			Base::exportDrawTag(base_dt::_dtag);
		}
};
