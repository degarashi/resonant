#include "test.hpp"
#include "scene.hpp"
#include "../util/dwrapper.hpp"
#include "sprite.hpp"
#include "../input.hpp"
#include "spinner/random.hpp"

const int N_Sprite = 5;
namespace {
	using CBStateInit = std::function<rs::DSortV ()>;

	const std::string c_name[N_Sprite] = {
		"z_desc",
		"techpass|z_asc|texture|buffer",
		"techpass|z_desc|texture|buffer",
		"buffer|z_asc|techpass|texture",
		"buffer|z_desc|techpass|texture"
	};
	const CBStateInit c_makeds[N_Sprite] = {
		[](){
			return rs::DSortV{rs::cs_dsort_z_desc};
		},
		// TechPass, ZAsc, Texture, Buffer
		[](){
			return rs::DSortV{rs::cs_dsort_techpass,
						rs::cs_dsort_z_asc,
						rs::cs_dsort_texture,
						rs::cs_dsort_buffer};
		},
		// TechPass, ZDsc, Texture, Buffer
		[](){
			return rs::DSortV{rs::cs_dsort_techpass,
						rs::cs_dsort_z_desc,
						rs::cs_dsort_texture,
						rs::cs_dsort_buffer};
		},
		// Buffer, ZAsc,  TechPass, Texture
		[](){
			return rs::DSortV{rs::cs_dsort_buffer,
						rs::cs_dsort_z_asc,
						rs::cs_dsort_techpass,
						rs::cs_dsort_texture};
		},
		// Buffer, ZDesc,  TechPass, Texture
		[](){
			return rs::DSortV{rs::cs_dsort_buffer,
						rs::cs_dsort_z_desc,
						rs::cs_dsort_techpass,
						rs::cs_dsort_texture};
		}
	};
}
struct Sc_DSort::St_Test : StateT<St_Test> {
	int	_index;
	St_Test(Sc_DSort& self, int index, bool /*bFirst*/, bool /*bSort*/) {
		_index = index;
		self._myDg->get()->setSortAlgorithm(c_makeds[index](), false);
	}
	void onUpdate(Sc_DSort& self) override {
		// 一定時間ごとにランダムなスプライトを選んで一旦グループから外し、再登録
		{
			int index = self._base.getRand().getUniform<int>({std::size_t(0), self._hlSpriteV.size()-1});
			auto& obj = self._hlSpriteV[index];
			auto* dr = self.getBase().getDraw()->get();
			dr->remObj(obj);
			dr->addObj(obj);
		}
		auto lk = sharedv.lock();
		// 他のテストへ切り替え
		for(int i=0 ; i<static_cast<int>(countof(c_makeds)) ; i++) {
			if(mgr_input.isKeyPressed(lk->actNumber[i])) {
				self.setStateNew<St_Test>(std::ref(static_cast<Sc_DSort&>(self)), i, false, false);
				return;
			}
		}
		// 他のシーンへ切り替え
		self._base.checkSwitchScene();
	}
	rs::LCValue recvMsg(Sc_DSort& /*self*/, rs::GMessageId id, const rs::LCValue& /*arg*/) override {
		if(id == MSG_StateName)
			return (boost::format("Sc_DSort: %1%") % c_name[_index]).str();
		return rs::LCValue();
	}
};
rs::HLTex Sc_DSort::LoadTexture(int index) {
	rs::HLTex hlST = mgr_gl.loadTexture((boost::format("spr%1%.png") % index).str());
	hlST->get()->setFilter(rs::IGLTexture::MipmapLinear, true, true);
	return hlST;
}
ImplDrawGroup(MyP, 0x2000)
ImplDrawGroup(MyDG, 0x1000)
struct Sc_DSort::St_Init : StateT<St_Init> {
	void onConnected(Sc_DSort& self, rs::HGroup /*hGroup*/) override {
		self.getDrawGroup().setSortAlgorithm({rs::cs_dsort_priority_asc}, false);
		// 前シーンのアップデートグループを流用
		{	auto hG = mgr_scene.getSceneBase(1).getUpdate();
			mgr_scene.getSceneBase(0).getUpdate()->get()->addObj(hG); }
		// ---- make FBClear ----
		self.getDrawGroup().addObj(MakeFBClear(0x0000));
		// 前シーンの描画グループを流用
		{	auto hDGroup = mgr_scene.getSceneBase(1).getDraw();
			self.getDrawGroup().addObj(rs_mgr_obj.makeDrawGroup<MyP>(hDGroup).first.get()); }
		self._myDg = rs_mgr_obj.makeDrawGroup<MyDG>().first;
		self.getDrawGroup().addObj(self._myDg.get());
		// スプライト画像の読み込み
		using SpriteObj = rs::util::DWrapper<Sprite>;
		for(int i=0 ; i<static_cast<int>(self._hlSpriteV.size()) ; i++) {
			rs::HLTex hlST = LoadTexture(i);
			auto hlp = rs_mgr_obj.makeDrawable<SpriteObj>(::MakeCallDraw<Engine>(), Sprite::T_Sprite, self._myDg, hlST, i*0.1f);
			hlp.second->setScale(spn::Vec2(0.3f));
			hlp.second->setOffset(spn::Vec2(-1.f + i*0.2f,
									(i&1)*-0.1f));
			self._hlSpriteV[i] = hlp.first;
			auto& upd = self.getBase().getUpdate().ref();
			upd.get()->addObj(hlp.first.get());
		}
		self.setStateNew<St_Test>(std::ref(self), 0, true, false);
	}
};
Sc_DSort::Sc_DSort(Sc_Base& b): _base(b), _hlSpriteV(N_Sprite) {
	setStateNew<St_Init>();
}
