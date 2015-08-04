#pragma once
#include "../updater.hpp"
#include "../gameloophelper.hpp"

namespace rs {
	class GLEffect;
	namespace util {
		template <class Base>
		class DWrapper : public DrawableObjT<DWrapper<Base>>,
						public Base,
						public spn::EnableFromThis<HDObj>
		{
			private:
				using CallDrawF = std::function<void (const Base&, IEffect& e)>;
				CallDrawF	_callDrawF;
				using base_dt = DrawableObjT<DWrapper<Base>>;
				IdValue		_tpId;
				WDGroup		_wDGroup;
				HDGroup _getDGroup() const {
					if(auto dg = _wDGroup.lock())
						return dg;
					return mgr_scene.getSceneBase().getDraw();
				}
			protected:
				struct St_Default : base_dt::template StateT<St_Default> {
					void onConnected(DWrapper& self, HGroup /*hGroup*/) override {
						auto hl = self.handleFromThis();
						self._getDGroup()->get()->addObj(hl);
					}
					void onDisconnected(DWrapper& self, HGroup /*hGroup*/) override {
						auto hl = self.handleFromThis();
						self._getDGroup()->get()->remObj(hl);
					}
					void onUpdate(DWrapper& self) override {
						self.refreshDrawTag();
					}
					void onDraw(const DWrapper& self, IEffect& e) const override {
						e.setTechPassId(self._tpId);
						self._callDrawF(self, e);
					}
				};
			public:
				template <class... Ts>
				DWrapper(const CallDrawF& cd, IdValue tpId, HDGroup hDg, Ts&&... ts):
					Base(std::forward<Ts>(ts)...),
					_callDrawF(cd),
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
