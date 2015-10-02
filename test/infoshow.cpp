#include "test.hpp"
#include "infoshow.hpp"
#include "../gpu.hpp"
#include "../gameloophelper.hpp"
#include "../differential.hpp"
#include "engine.hpp"
#include "../util/dwrapper.hpp"
#include "../util/screenrect.hpp"

const rs::IdValue InfoShow::T_Info = GlxId::GenTechId("Text", "Default");
const std::string g_fontName("IPAGothic");
// ---------------------- InfoShow::MySt ----------------------
struct InfoShow::MySt : StateT<MySt> {
	rs::util::WindowRect* pRect;
	rs::LCValue recvMsg(InfoShow& self, const rs::GMessageStr& msg, const rs::LCValue& arg) override;
	void onConnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDisconnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDraw(const InfoShow& self, rs::IEffect& e) const override;
	void onUpdate(InfoShow& self, const rs::SPLua& ls) override;
};
void InfoShow::MySt::onDraw(const InfoShow& self, rs::IEffect& e) const {
	int fps;
	{
		auto lkb = sharedbase.lockR();
		// FPSの表示
		fps = lkb->fps.getFPS();
	}
	std::stringstream ss;
	ss << "FPS: " << fps << std::endl;
	// TODO: Luaへのメッセージ送信機構を後で実装して対応
	// rs::Object& obj = *mgr_scene.getScene(0).ref();
	// ss << "State: " << obj.recvMsg(MSG_StateName).toString() << std::endl;
	// バッファ切り替えカウント数の表示
	auto& buffd = self._count.buffer;
	ss << "VertexBuffer: " << buffd.vertex << std::endl;
	ss << "IndexBuffer: " << buffd.index << std::endl;
	ss << "DrawIndexed: " << self._count.drawIndexed << std::endl;
	ss << "DrawNoIndexed: " << self._count.drawNoIndexed << std::endl;

	auto& ths = const_cast<InfoShow&>(self);
	e.setTechPassId(T_Info);
	ths._textHud.setText(self._infotext+ spn::Text::UTFConvertTo32(ss.str()).c_str());
	self._textHud.draw(e);
}
#include "spinner/structure/profiler.hpp"
#include <thread>
void InfoShow::MySt::onUpdate(InfoShow& self, const rs::SPLua& /*ls*/) {
	{
		auto lk = sharedbase.lockR();
		self._count = lk->diffCount;
	}
	self._textHud.exportDrawTag(self._dtag);
	self._dtag.idTechPass = InfoShow::T_Info;
	auto sz = self._textHud.getText()->getSize();
	pRect->setScale({sz.width, -sz.height});
	pRect->setOffset(self._offset);
}

const rs::IdValue T_Rect = GlxId::GenTechId("Sprite", "Rect");
void InfoShow::MySt::onConnected(InfoShow& self, rs::HGroup) {
	auto* d = self._hDg->get();
	{
		auto hlp = rs_mgr_obj.makeDrawable<rs::util::DWrapper<rs::util::WindowRect>>(::MakeCallDraw<Engine>(), T_Rect, rs::HDGroup());
		hlp.second->setAlpha(0.5f);
		hlp.second->setColor({0,1,0});
		hlp.second->setDepth(1.f);
		hlp.second->setPriority(self._dtag.priority-1);
		d->addObj(hlp.first);
		pRect = hlp.second;
	}
	auto lh = self.handleFromThis();
	d->addObj(lh);
}
void InfoShow::MySt::onDisconnected(InfoShow& self, rs::HGroup) {
	auto* d = self._hDg->get();
	auto lh = self.handleFromThis();
	d->remObj(lh);
}
rs::LCValue InfoShow::MySt::recvMsg(InfoShow& /*self*/, const rs::GMessageStr& /*msg*/, const rs::LCValue& /*arg*/) {
	return rs::LCValue();
}
// ---------------------- InfoShow ----------------------
InfoShow::InfoShow(rs::HDGroup hDg, rs::Priority dprio):
	_hDg(hDg),
	_offset(0)
{
	//  フォント読み込み
	rs::CCoreID cid = mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, 20, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel));
	_textHud.setCCoreId(cid);
	_textHud.setDepth(0.f);
	_dtag.priority = dprio;

	rs::GPUInfo info;
	info.onDeviceReset();
	std::stringstream ss;
	ss << "Version: " << info.version() << std::endl
			<< "GLSL Version: " << info.glslVersion() << std::endl
			<< "Vendor: " << info.vendor() << std::endl
			<< "Renderer: " << info.renderer() << std::endl
			<< "DriverVersion: " << info.driverVersion() << std::endl;
	_infotext = spn::Text::UTFConvertTo32(ss.str());
	setStateNew<MySt>();
}
void InfoShow::setOffset(const spn::Vec2& ofs) {
	_offset = ofs;
	_textHud.setWindowOffset(ofs);
}
rs::Priority InfoShow::getPriority() const { return 0x1000; }

#include "../updater_lua.hpp"
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, InfoShow, InfoShow, "Object", NOTHING, (setOffset)(getPriority), (rs::HDGroup)(rs::Priority))
