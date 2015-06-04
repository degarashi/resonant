#include "test.hpp"
#include "profileshow.hpp"
#include "infoshow.hpp"
#include "engine.hpp"
#include "../gameloophelper.hpp"
#include <iomanip>

// ---------------------- ProfileShow::St_Default ----------------------
struct ProfileShow::St_Default : StateT<St_Default> {
	void onConnected(ProfileShow& self, rs::HGroup hGroup) override {
		self._dtag.zOffset = 0.f;
		auto d = mgr_scene.getSceneBase().getDraw();
		d->get()->addObj(self.handleFromThis());
	}
	void onDisconnected(ProfileShow& self, rs::HGroup hGroup) override {
		auto d = mgr_scene.getSceneBase().getDraw();
		d->get()->remObj(self.handleFromThis());
	}
	void onDraw(const ProfileShow& self) const override {
		if(self._spProfile) {
			auto lk = sharedv.lock();
			auto& fx = *lk->pEngine;
			fx.setTechPassId(InfoShow::T_Info);
			auto lkb = sharedbase.lock();
			auto tsz = lkb->screenSize;
			auto fn = [tsz](int x, int y, float r) {
				float rx = spn::Rcp22Bit(tsz.width/2),
					  ry = spn::Rcp22Bit(tsz.height/2);
				return spn::Mat33(rx*r,		0,			0,
								0,			ry*r, 		0,
								-1.f+x*rx,	1.f-y*ry,	1);
			};
			fx.setUniform(InfoShow::U_Text, fn(100,320,1), true);

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
			self._hlText = mgr_text.createText(self._charId, ss.str());
			self._hlText->draw(&fx);
		}
	}
	void onUpdate(ProfileShow& self) override {
		// プロファイラの(1フレーム前の)情報を取得
		self._spProfile = spn::profiler.getRoot();
	}
};

// ---------------------- ProfileShow ----------------------
ProfileShow::ProfileShow(rs::CCoreID cid): _charId(cid) {}
void ProfileShow::initState() {
	setStateNew<St_Default>();
}
rs::Priority ProfileShow::getPriority() const { return 0x2000; }
