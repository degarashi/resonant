#include "test.hpp"
#include "scene.hpp"
#include "dwrapper.hpp"
#include "sprite.hpp"
#include "../input.hpp"

namespace {
	using CBStateInit = std::function<void (Sc_DSort&,bool)>;
	DefineGroupT(MyDrawGroup, rs::DrawGroup)
}
const std::string c_name[5] = {
	"z_desc",
	"techpass|z_asc|texture|buffer",
	"techpass|z_desc|texture|buffer",
	"buffer|z_asc|techpass|texture",
	"buffer|z_desc|techpass|texture"
};
const CBStateInit c_init[5] = {
	[](Sc_DSort& self, bool bSort){
		self.getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(
					rs::DSortV{rs::cs_dsort_z_desc}, bSort));
	},
	// TechPass, ZAsc, Texture, Buffer
	[](Sc_DSort& self, bool bSort){
		self.getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(
					rs::DSortV{rs::cs_dsort_techpass,
								rs::cs_dsort_z_asc,
								rs::cs_dsort_texture,
								rs::cs_dsort_buffer}, bSort));
	},
	// TechPass, ZDsc, Texture, Buffer
	[](Sc_DSort& self, bool bSort){
		self.getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(
					rs::DSortV{rs::cs_dsort_techpass,
								rs::cs_dsort_z_desc,
								rs::cs_dsort_texture,
								rs::cs_dsort_buffer}, bSort));
	},
	// Buffer, ZAsc,  TechPass, Texture
	[](Sc_DSort& self, bool bSort){
		self.getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(
					rs::DSortV{rs::cs_dsort_buffer,
								rs::cs_dsort_z_asc,
								rs::cs_dsort_techpass,
								rs::cs_dsort_texture}, bSort));
	},
	// Buffer, ZDesc,  TechPass, Texture
	[](Sc_DSort& self, bool bSort){
		self.getBase().setDraw(rs_mgr_obj.makeDrawGroup<MyDrawGroup>(
					rs::DSortV{rs::cs_dsort_buffer,
								rs::cs_dsort_z_desc,
								rs::cs_dsort_techpass,
								rs::cs_dsort_texture}, bSort));
	}
};
struct Sc_DSort::St_Test : StateT<St_Test> {
	int	_index;
	St_Test(Sc_DSort& self, int index, bool bSort) {
		_index = index;
		c_init[index](self, bSort);
		// 前シーンの描画 & アップデートグループを流用
		{	 auto hDGroup = mgr_scene.getSceneBase(1).getDraw();
			mgr_scene.getSceneBase(0).getDraw()->get()->addObj(hDGroup); }
		{	auto hG = mgr_scene.getSceneBase(1).getUpdate();
			mgr_scene.getSceneBase(0).getUpdate()->get()->addObj(hG); }
		auto& upd = mgr_scene.getSceneBase(0).getUpdate().ref();
		for(auto& hl : self._hlSprite)
			upd.get()->addObj(hl.get());
	}
	void onUpdate(Sc_Base& self) override {
		auto lk = sharedv.lock();
		// 他のテストへ切り替え
		for(int i=0 ; i<countof(c_init) ; i++) {
			if(mgr_input.isKeyPressed(lk->actNumber[i])) {
				self.setStateNew<St_Test>(std::ref(static_cast<Sc_DSort&>(self)), i, false);
				return;
			}
		}
		// 他のシーンへ切り替え
		if(mgr_input.isKeyPressed(lk->actSound))
			mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Sound>(), true);
		else if(mgr_input.isKeyPressed(lk->actCube))
			mgr_scene.setPushScene(rs_mgr_obj.makeObj<Sc_Cube>(), true);
		else
			self.checkQuit();
	}
	rs::LCValue recvMsg(Sc_Base& self, rs::GMessageId id, const rs::LCValue& arg) override {
		if(id == MSG_StateName)
			return (boost::format("Sc_DSort: %1%") % c_name[_index]).str();
		return rs::LCValue();
	}
};
struct Sc_DSort::St_Init : StateT<St_Init> {
	void onConnected(Sc_Base& self, rs::HGroup hGroup) override {
		// スプライト画像の読み込み
		auto& ths = static_cast<Sc_DSort&>(self);
		using SpriteObj = DWrapper<Sprite>;
		for(int i=0 ; i<5 ; i++) {
			rs::HLTex hlST = mgr_gl.loadTexture((boost::format("spr%1%.png") % i).str());
			hlST->get()->setFilter(rs::IGLTexture::MipmapLinear, true, true);
			rs::HLDObj hlObj = rs_mgr_obj.makeDrawable<SpriteObj>(Sprite::T_Sprite, hlST, i*0.1f);
			auto* sp = static_cast<SpriteObj*>(hlObj->get());
			sp->setScale(spn::Vec2(0.3f));
			sp->setOffset(spn::Vec2(-1.f + i*0.2f,
									(i&1)*-0.1f));
			ths._hlSprite[i] = hlObj;
		}
		self.setStateNew<St_Test>(std::ref(ths), 0, false);
	}
};
void Sc_DSort::initState() {
	setStateNew<St_Init>();
}
