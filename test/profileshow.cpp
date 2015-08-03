#include "test.hpp"
#include "profileshow.hpp"
#include "infoshow.hpp"
#include "engine.hpp"
#include "../gameloophelper.hpp"
#include <iomanip>
#include "../util/dwrapper.hpp"

// ---------------------- ProfileShow::St_Default ----------------------
struct ProfileShow::St_Default : StateT<St_Default> {
	rs::util::WindowRect* pRect;
	void onConnected(ProfileShow& self, rs::HGroup /*hGroup*/) override {
		auto* dg = self._hDg->get();
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::DWrapper<rs::util::WindowRect>>(::MakeCallDraw<Engine>(), T_Rect, rs::HDGroup());
			hlp.second->setColor({0,0,1});
			hlp.second->setAlpha(0.5f);
			dg->addObj(hlp.first);
			pRect = hlp.second;
		}
		dg->addObj(self.handleFromThis());
	}
	void onDisconnected(ProfileShow& self, rs::HGroup /*hGroup*/) override {
		auto* dg = self._hDg->get();
		dg->remObj(self.handleFromThis());
	}
	void onDraw(const ProfileShow& self, rs::GLEffect& e) const override {
		if(self._spProfile) {
			std::stringstream ss;
			ss.precision(2);
			ss << std::setfill('0') << std::setw(5);
			auto& hist = spn::profiler.getIntervalHistory();
			self._spProfile->iterateDepthFirst<false>([&ss, &hist](const spn::Profiler::Block& nd, int depth){
				auto us = nd.hist.tAccum - nd.getLowerTime();
				for(int i=0 ; i<depth ; i++)
					ss << "---";
				int tavg = 0;
				if(hist.count(nd.layerHistId) > 0) {
					tavg = hist.at(nd.layerHistId).getAverageTime().count();
				}
				std::string s = (boost::format("%1%: %2$2.2fms,%|30t|N=%3%,%|40t|Avg=%4%") % nd.name % ((us.count()/10)/100.f) % nd.hist.nCalled % tavg).str();
				ss << s << std::endl;
				return spn::Iterate::StepIn;
			});
			const_cast<ProfileShow&>(self)._textHud.setText(ss.str());
			e.setTechPassId(InfoShow::T_Info);
			self._textHud.draw(e);
		}
	}
	void onUpdate(ProfileShow& self) override {
		// プロファイラの(1フレーム前の)情報を取得
		self._spProfile = spn::profiler.getRoot();
		auto sz = self._textHud.getText()->getSize();
		pRect->setScale({sz.width, -sz.height});
		pRect->setOffset(self._offset);
	}
};

// ---------------------- ProfileShow ----------------------
ProfileShow::ProfileShow(rs::CCoreID cid, rs::HDGroup hDg):
	_hDg(hDg),
	_offset(0)
{
	_textHud.setCCoreId(cid);
	_textHud.setScreenOffset({-1,0});
	_textHud.setDepth(0.f);
	setStateNew<St_Default>();
}
void ProfileShow::setOffset(const spn::Vec2& ofs) {
	_offset = ofs;
	_textHud.setWindowOffset(ofs);
}
rs::Priority ProfileShow::getPriority() const { return 0x2000; }
