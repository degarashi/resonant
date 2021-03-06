#include "profileshow.hpp"
#include "screenrect.hpp"
#include "dwrapper.hpp"
#include "../gameloophelper.hpp"
#include <iomanip>

namespace rs {
	namespace util {
		// ---------------------- ProfileShow::St_Default ----------------------
		struct ProfileShow::St_Default : StateT<St_Default> {
			HLDObj hlRect;
			WindowRect& _getWRect() const {
				return *static_cast<DWrapper<WindowRect>*>(hlRect->get());
			}
			void onConnected(ProfileShow& self, HGroup /*hGroup*/) override {
				auto* dg = self._hDg->get();
				{
					auto fn = [](const auto& t, IEffect& e) { t.draw(e); };
					auto hlp = rs_mgr_obj.makeDrawable<DWrapper<WindowRect>>(fn, self._idRect, HDGroup());
					hlp.second->setColor({0,0,1});
					hlp.second->setAlpha(0.5f);
					hlp.second->setDepth(1.f);
					hlp.second->setPriority(self._dtag.priority-1);
					dg->addObj(hlp.first);
					hlRect = hlp.first;
				}
				dg->addObj(HDObj::FromHandle(self.handleFromThis()));
			}
			void onDisconnected(ProfileShow& self, HGroup /*hGroup*/) override {
				auto* dg = self._hDg->get();
				dg->remObj(HDObj::FromHandle(self.handleFromThis()));
			}
			void onDraw(const ProfileShow& self, IEffect& e) const override {
				if(self._spProfile) {
					std::stringstream ss;
					ss.precision(2);
					ss << std::setfill('0') << std::setw(5);
					self._spProfile->iterateDepthFirst<false>([&ss](const spn::prof::Profiler::Block& nd, const int depth){
						const auto us = nd.hist.tAccum - nd.getLowerTime();
						for(int i=0 ; i<depth ; i++)
							ss << "---";
						const std::string s = (
								boost::format("%1%: %2$2.2fms,%|30t|N=%3%,%|40t|Avg=%4%")
									% nd.name
									% ((us.count()/10)/100.f)	// us -> ms表示
									% nd.hist.nCalled
									% nd.getAverageTime(true).count()
							).str();
						ss << s << std::endl;
						return spn::Iterate::StepIn;
					});
					e.setTechPassId(self._idText);
					const_cast<ProfileShow&>(self)._textHud.setText(ss.str());
					self._textHud.draw(e);
				}
			}
			void onUpdate(ProfileShow& self) override {
				// プロファイラの(1フレーム前の)情報を取得
				self._spProfile = spn::profiler.getPrev().root;
				auto sz = self._textHud.getText()->getSize();
				auto& r = _getWRect();
				r.setScale({sz.width, -sz.height});
				r.setOffset(self._offset);
			}
		};

		// ---------------------- ProfileShow ----------------------
		ProfileShow::ProfileShow(IdValue idText, IdValue idRect, CCoreID cid, HDGroup hDg, Priority uprio, Priority dprio):
			_idText(idText),
			_idRect(idRect),
			_hDg(hDg),
			_uprio(uprio),
			_offset(0)
		{
			_dtag.priority = dprio;
			_textHud.setCCoreId(cid);
			_textHud.setScreenOffset({-1,0});
			_textHud.setDepth(0.f);
			setStateNew<St_Default>();
		}
		void ProfileShow::setOffset(const spn::Vec2& ofs) {
			_offset = ofs;
			_textHud.setWindowOffset(ofs);
		}
		Priority ProfileShow::getPriority() const {
			return _uprio;
		}
	}
}
