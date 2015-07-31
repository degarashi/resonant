#pragma once
#include "updater.hpp"
#include "gameloophelper.hpp"

namespace rs {
	class GLEffect;
	namespace util {
		void DWrapper_SetTPId(GLEffect& e, IdValue id);

		template <class Base, class Convert>
		class DWrapper : public DrawableObjT<DWrapper<Base, Convert>>,
						public Base,
						public spn::EnableFromThis<HDObj>
		{
			private:
				using base_dt = DrawableObjT<DWrapper<Base, Convert>>;
				IdValue		_tpId;
				WDGroup		_wDGroup;
				HDGroup _getDGroup() const {
					if(auto dg = _wDGroup.lock())
						return dg;
					return mgr_scene.getSceneBase().getDraw();
				}
			protected:
				struct St_Default : base_dt::template StateT<St_Default> {
					void onConnected(DWrapper& self, HGroup hGroup) override {
						auto hl = self.handleFromThis();
						self._getDGroup()->get()->addObj(hl);
					}
					void onDisconnected(DWrapper& self, HGroup hGroup) override {
						auto hl = self.handleFromThis();
						self._getDGroup()->get()->remObj(hl);
					}
					void onUpdate(DWrapper& self) override {
						self.refreshDrawTag();
					}
					void onDraw(const DWrapper& self, GLEffect& e) const override {
						DWrapper_SetTPId(e, self._tpId);
						self.Base::draw(Convert()(e));
					}
				};
			public:
				template <class... Ts>
				DWrapper(IdValue tpId, HDGroup hDg, Ts&&... ts):
					Base(std::forward<Ts>(ts)...),
					_tpId(tpId)
				{
					if(hDg)
						_wDGroup = hDg.weak();
					refreshDrawTag();
					base_dt::_dtag.idTechPass = tpId;
					base_dt::template setStateNew<St_Default>();
				}
				void setPriority(Priority p) {
					base_dt::_dtag.priority = p;
				}
				void refreshDrawTag() {
					Base::exportDrawTag(base_dt::_dtag);
				}
		};
	}
}
