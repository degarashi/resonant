#include "test.hpp"
#include "scene.hpp"
#include "../util/dwrapper.hpp"
#include "sprite.hpp"
#include "spinner/random.hpp"
#include "../util/screen.hpp"
#include "../input.hpp"
#include "../sys_uniform_value.hpp"
#include "engine.hpp"

class BoundingSprite : public rs::util::DWrapper<Sprite> {
	private:
		using base_t = rs::util::DWrapper<Sprite>;
		spn::Vec2	_svec;
		struct St_Default : base_t::St_Default {
			void onUpdate(base_t& self, const rs::SPLua& /*ls*/) override {
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
			base_t(::MakeCallDraw<Engine>(), tpId, hDg, hTex, 0.f),
			_svec(svec)
		{
			setOffset(pos);
			setZRange({-1.f, 1.f});
			setStateNew<St_Default>();
		}
};
enum CPriority : rs::Priority {
	p_fbsw0,
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
namespace {
	const rs::IdValue T_PostEffect = rs::IEffect::GlxId::GenTechId("PostEffect", "Default");
}
struct Sc_DSortD::St_Default : StateT<St_Default> {
	rs::HLFb	_hlFb;
	rs::HLTex	_hlTex[2];
	rs::util::PostEffect	*_pBlur0,
							*_pBlur1;
	int swt = 0;
	bool bBlur = false;
	St_Default() {
		auto lk = sharedbase.lockR();
		auto s = lk->screenSize;
		_hlFb = mgr_gl.makeFBuffer();
		auto hlDp = mgr_gl.createTexture(s, GL_RGBA8, false, false);
		_hlFb->get()->attachTexture(rs::GLFBuffer::Att::DEPTH, hlDp);
		for(auto& ht : _hlTex)
			ht = mgr_gl.createTexture(s, GL_RGBA8, false, false);
	}
	void onConnected(Sc_DSortD& self, rs::HGroup /*hGroup*/) override {
		auto	&sb0 = mgr_scene.getSceneInterface(0),
				&sb1 = mgr_scene.getSceneInterface(1);
		auto &dg0 = sb0.getDrawGroup()->operator *();
		// 前シーンのアップデートグループを流用
		{	auto hG = sb1.getUpdGroup();
			sb0.getUpdGroup()->get()->addObj(hG); }

		// メイン描画グループ (ユーザー定義の優先度でソート)
		dg0.setSortAlgorithm({rs::cs_dsort_priority_asc}, false);
		// [FBSwitch&Clear(Cur)]
		{
			rs::draw::ClearParam cp{spn::Vec4(0,0,0,1.f), 1.f, 0};
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::FBSwitch>(p_fbsw0, _hlFb, cp);
			dg0.addObj(hlp.first);
		}
		// [現シーンのDG] Z値でソート(Dynamic)
		auto hDG = rs_mgr_obj.makeDrawGroup<MyDGroup>(rs::DSortV{rs::cs_dsort_z_asc}, true);
		dg0.addObj(hDG.first.get());
		// [Blur(前回の重ね)]
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::PostEffect>(T_PostEffect, p_blur0);
			hlp.second->setParam(rs::unif::Alpha, 0.f);
			dg0.addObj(hlp.first);
			_pBlur0 = hlp.second;
		}
		// [FBSwitch&ClearZ(Default)]
		{
			rs::draw::ClearParam clp{spn::none, 1.f, {}};
			dg0.addObj(rs_mgr_obj.makeDrawable<rs::util::FBSwitch>(p_fbsw1, rs::HFb(), clp).first);
		}
		// [Blur(今回のベタ描画)]
		{
			auto hlp = rs_mgr_obj.makeDrawable<rs::util::PostEffect>(T_PostEffect, p_blur1);
			hlp.second->setParam(rs::unif::Alpha, 1.f);
			dg0.addObj(hlp.first);
			_pBlur1 = hlp.second;
		}
		// [前シーンのDG]
		dg0.addObj(rs_mgr_obj.makeDrawGroup<PrevDGroup>(sb1.getDrawGroup()).first.get());

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
		auto upd = sb0.getUpdGroup();
		std::vector<rs::HLTex> tex(N_Sprite);
		for(int i=0 ; i<N_Sprite ; i++)
			tex[i] = Sc_DSort::LoadTexture(i);
		for(int i=0 ; i<150 ; i++) {
			auto hlp = rs_mgr_obj.makeDrawable<BoundingSprite>(Sprite::T_Sprite, hDG.first, tex[i % N_Sprite], fnPos(), fnSV());
			hlp.second->setScale(spn::Vec2(0.2f));
			upd->get()->addObj(hlp.first.get());
		}
	}
	void onDraw(const Sc_DSortD& /*self*/, rs::IEffect& /*e*/) const override {
		_hlFb->get()->attachTexture(rs::GLFBuffer::Att::COLOR0, _hlTex[swt]);
		_pBlur0->setParam(rs::unif::texture::Diffuse, _hlTex[swt ^ 1]);
		_pBlur1->setParam(rs::unif::texture::Diffuse, _hlTex[swt]);
		const_cast<St_Default&>(*this).swt ^= 1;
	}
	void onUpdate(Sc_DSortD& self, const rs::SPLua& /*ls*/) override {
		auto lk = sharedv.lock();
		if(lk->actNumber[0]->isKeyPressed()) {
			bBlur = bBlur^1;
			_pBlur0->setParam(rs::unif::Alpha, (bBlur) ? 0.9f : 0.f);
		}
		// 他のシーンへ切り替え
		self._base.checkSwitchScene();
	}
	rs::LCValue recvMsg(Sc_DSortD& /*self*/, rs::GMessageId id, const rs::LCValue& /*arg*/) override {
		if(id == MSG_StateName)
			return "Sc_DSort(Dynamic)";
		return rs::LCValue();
	}
};
Sc_DSortD::Sc_DSortD(Sc_Base& b): _base(b) {
	setStateNew<St_Default>();
}
