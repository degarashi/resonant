#include "test.hpp"
#include "scene.hpp"
#include "dwrapper.hpp"
#include "sprite.hpp"
#include "spinner/random.hpp"
#include "../util/screen.hpp"
#include "../input.hpp"

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
	public:
		BoundingSprite(rs::IdValue tpId, rs::HDGroup hDg, rs::HTex hTex, const spn::Vec2& pos, const spn::Vec2& svec):
			base_t(tpId, hDg, hTex, 0.f),
			_svec(svec)
		{
			setOffset(pos);
			setZRange({-1.f, 1.f});
			setStateNew<St_Default>();
		}
};
enum CPriority : rs::Priority {
	p_fbsw0,
	p_fbclear,
	p_curScene,
	p_blur0,
	p_fbsw1,
	p_blur1,
	p_prevScene
};
DefineDrawGroupProxy(PrevDGroup)
ImplDrawGroup(PrevDGroup, p_prevScene)
DefineDrawGroup(MyDGroup)
ImplDrawGroup(MyDGroup, p_curScene)
struct Sc_DSortD::St_Default : StateT<St_Default> {
	rs::HLFb	_hlFb;
	rs::HLTex	_hlTex[2];
	rs::util::PostEffect	*_pBlur0,
							*_pBlur1;
	int swt = 0;
	bool bBlur = false;
	St_Default() {
		auto lk = sharedbase.lock();
		auto s = lk->screenSize;
		_hlFb = mgr_gl.makeFBuffer();
		auto hlDp = mgr_gl.createTexture(s, GL_RGBA8, false, false);
		_hlFb->get()->attach(rs::GLFBuffer::Att::DEPTH, hlDp);
		for(auto& ht : _hlTex)
			ht = mgr_gl.createTexture(s, GL_RGBA8, false, false);
	}
	void onConnected(Sc_DSortD& self, rs::HGroup hGroup) override {
		auto	&sb0 = mgr_scene.getSceneBase(0),
				&sb1 = mgr_scene.getSceneBase(1);
		// 前シーンのアップデートグループを流用
		{	auto hG = sb1.getUpdate();
			sb0.getUpdate()->get()->addObj(hG); }

		auto &dg0 = sb0.getDraw()->operator *();
		// メイン描画グループ (ユーザー定義の優先度でソート)
		dg0.setSortAlgorithm({rs::cs_dsort_priority_asc}, false);
		// [FBSwitch(Cur)]
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::FBSwitch>(p_fbsw0, _hlFb);
			self.getDrawGroup().addObj(hlp.first);
		}
		// [FBClear]
		{
			rs::draw::ClearParam cp{spn::Vec4(0,0,0,1.f), 1.f, 0};
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::FBClear>(p_fbclear, cp);
			self.getDrawGroup().addObj(hlp.first);
		}
		// [現シーンのDG] Z値でソート(Dynamic)
		auto hDG = rs_mgr_obj.makeDrawGroup<MyDGroup>(rs::DSortV{rs::cs_dsort_z_asc}, true);
		dg0.addObj(hDG.first.get());
		// [Blur(前回の重ね)]
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::PostEffect>(p_blur0);
			hlp.second->setAlpha(0);
			self.getDrawGroup().addObj(hlp.first);
			_pBlur0 = hlp.second;
		}
		// [FBSwitch(Default)]
		self.getDrawGroup().addObj(rs_mgr_obj.makeDrawable<rs::util::FBSwitch>(p_fbsw1, rs::HFb()).first);
		// [Blur(今回のベタ描画)]
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::PostEffect>(p_blur1);
			hlp.second->setAlpha(1.f);
			self.getDrawGroup().addObj(hlp.first);
			_pBlur1 = hlp.second;
		}
		// [前シーンのDG]
		dg0.addObj(rs_mgr_obj.makeDrawGroup<PrevDGroup>(sb1.getDraw()).first.get());

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
		// 引数に追加する描画グループハンドルを渡す？
		auto upd = sb0.getUpdate();
		for(int i=0 ; i<50 ; i++) {
			auto hlTex = Sc_DSort::LoadTexture(i % N_Sprite);
			auto hlp = rs_mgr_obj.makeDrawable<BoundingSprite>(Sprite::T_Sprite, hDG.first, hlTex, fnPos(), fnSV());
			hlp.second->setScale(spn::Vec2(0.2f));
			upd->get()->addObj(hlp.first.get());
		}
	}
	void onDraw(const Sc_DSortD& self, rs::GLEffect& e) const override {
		_hlFb->get()->attach(rs::GLFBuffer::Att::COLOR0, _hlTex[swt]);
		_pBlur0->setTexture(rs::unif::texture::Diffuse, _hlTex[swt ^ 1]);
		_pBlur1->setTexture(rs::unif::texture::Diffuse, _hlTex[swt]);
		const_cast<St_Default&>(*this).swt ^= 1;
	}
	void onUpdate(Sc_DSortD& self) override {
		auto lk = sharedv.lock();
		if(mgr_input.isKeyPressed(lk->actNumber[0])) {
			bBlur = bBlur^1;
			_pBlur0->setAlpha((bBlur) ? 0.9f : 0.f);
		}
		// 他のシーンへ切り替え
		self._base.checkSwitchScene();
	}
	rs::LCValue recvMsg(Sc_DSortD& self, rs::GMessageId id, const rs::LCValue& arg) override {
		if(id == MSG_StateName)
			return "Sc_DSort(Dynamic)";
		return rs::LCValue();
	}
};
Sc_DSortD::Sc_DSortD(Sc_Base& b): _base(b) {
	setStateNew<St_Default>();
}
