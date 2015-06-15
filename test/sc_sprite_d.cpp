#include "test.hpp"
#include "scene.hpp"
#include "dwrapper.hpp"
#include "sprite.hpp"
#include "../spinner/random.hpp"

class BoundingSprite : public DWrapper<Sprite> {
	private:
		using base_t = DWrapper<Sprite>;
		spn::Vec2	_svec;
		struct St_Default : base_t::St_Default {
			void onUpdate(base_t& self) override {
				auto& ths = static_cast<BoundingSprite&>(self);
				auto& sc = self.getScale();
				auto& ofs = self.refOffset();
				auto& svec = ths._svec;
				ofs += svec;
				// 境界チェック
				// Left
				if(ofs.x < -1.f) {
					svec.x *= -1.f;
					ofs.x = -1.f;
				}
				// Right
				if(ofs.x + sc.x > 1.f) {
					svec.x *= -1.f;
					ofs.x = 1.f - sc.x;
				}
				// Top
				if(ofs.y + sc.y > 1.f) {
					svec.y *= -1.f;
					ofs.y = 1.f - sc.y;
				}
				// Bottom
				if(ofs.y < -1.f) {
					svec.y *= -1.f;
					ofs.y = -1.f;
				}

				// ZOffset = y
				self.setZOffset(ofs.y);
				self.refreshDrawTag();
			}
		};
	protected:
		void initState() override {
			base_t::initState();
			setStateNew<St_Default>();
		}
	public:
		BoundingSprite(rs::IdValue tpId, rs::HTex hTex, const spn::Vec2& pos, const spn::Vec2& svec):
			base_t(tpId, hTex, 0.f),
			_svec(svec)
		{
			setOffset(pos);
			setZRange({-1.f, 1.f});
		}
};
struct Sc_DSortD::St_Default : StateT<St_Default> {
	void onConnected(Sc_DSortD& self, rs::HGroup hGroup) override {
		auto& sb0 = self.getBase();
		auto& sb1 = mgr_scene.getSceneBase(1);
		// Z値でソート(Dynamic)
		sb0.getDraw()->get()->setSortAlgorithm({rs::cs_dsort_z_asc}, true);
		// 前シーンのアップデートグループを流用
		{	auto hG = sb1.getUpdate();
			sb0.getUpdate()->get()->addObj(hG); }
		// 前シーンの描画グループを流用
		{	auto hDGroup = sb1.getDraw();
			sb0.getDraw()->get()->addObj(hDGroup); }

		// スプライト読み込み
		// ランダムで位置とスピードを設定
		auto& rnd = self._base.getRand();
		auto fnPos = [&rnd]() -> spn::Vec2 {
			return {rnd.template getUniform<float>({-1.f, 1.f}),
					rnd.template getUniform<float>({-1.f, 1.f})};
		};
		auto fnSV = [&rnd]() -> spn::Vec2 {
			return {rnd.template getUniform<float>({-0.01f, 0.01f}),
					rnd.template getUniform<float>({-0.005f, 0.005f})};
		};
		auto upd = sb0.getUpdate();
		for(int i=0 ; i<50 ; i++) {
			auto hlTex = Sc_DSort::LoadTexture(i % N_Sprite);
			rs_mgr_obj.makeDrawable<BoundingSprite>(Sprite::T_Sprite, hlTex, fnPos(), fnSV());
			auto hlp = rs_mgr_obj.makeDrawable<BoundingSprite>(Sprite::T_Sprite, hlTex, fnPos(), fnSV());
			hlp.second->setScale(spn::Vec2(0.2f));
			upd->get()->addObj(hlp.first.get());
		}
	}
	void onUpdate(Sc_DSortD& self) override {
		// 他のシーンへ切り替え
		self._base.checkSwitchScene();
	}
	rs::LCValue recvMsg(Sc_DSortD& self, rs::GMessageId id, const rs::LCValue& arg) override {
		if(id == MSG_StateName)
			return "Sc_DSort(Dynamic)";
		return rs::LCValue();
	}
};
Sc_DSortD::Sc_DSortD(Sc_Base& b): _base(b) {}
void Sc_DSortD::initState() {
	setStateNew<St_Default>();
}
