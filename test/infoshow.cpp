#include "test.hpp"
#include "infoshow.hpp"
#include "../gpu.hpp"
#include "../gameloophelper.hpp"
#include "../differential.hpp"
#include "engine.hpp"

const rs::IdValue InfoShow::T_Info = GlxId::GenTechId("TheTech", "P2");
// ---------------------- InfoShow::MySt ----------------------
struct InfoShow::MySt : StateT<MySt> {
	rs::LCValue recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) override;
	void onConnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDisconnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDraw(const InfoShow& self, rs::GLEffect& e) const override;
	void onUpdate(InfoShow& self) override;
};
void InfoShow::MySt::onDraw(const InfoShow& self, rs::GLEffect& e) const {
	auto lkb = sharedbase.lock();
	// FPSの表示
	int fps = lkb->fps.getFPS();
	std::stringstream ss;
	ss << "FPS: " << fps << std::endl;
	rs::Object& obj = *mgr_scene.getScene(0).ref();
	ss << "State: " << obj.recvMsg(MSG_StateName).toString() << std::endl;
	// バッファ切り替えカウント数の表示
	auto& buffd = self._count.buffer;
	ss << "VertexBuffer: " << buffd.vertex << std::endl;
	ss << "IndexBuffer: " << buffd.index << std::endl;
	ss << "DrawIndexed: " << self._count.drawIndexed << std::endl;
	ss << "DrawNoIndexed: " << self._count.drawNoIndexed << std::endl;

	auto& ths = const_cast<InfoShow&>(self);
	ths._textHud.setText(self._infotext+ spn::Text::UTFConvertTo32(ss.str()).c_str());
	self._textHud.draw(e);
}
#include "../spinner/structure/profiler.hpp"
#include <thread>
void InfoShow::MySt::onUpdate(InfoShow& self) {
	auto lk = sharedbase.lock();
	self._count = lk->diffCount;

	self._textHud.exportDrawTag(self._dtag);
}
void InfoShow::MySt::onConnected(InfoShow& self, rs::HGroup) {
	auto lh = self.handleFromThis();
	auto d = mgr_scene.getSceneBase().getDraw();
	d->get()->addObj(lh);
}
void InfoShow::MySt::onDisconnected(InfoShow& self, rs::HGroup) {
	auto lh = self.handleFromThis();
	auto d = mgr_scene.getSceneBase().getDraw();
	d->get()->remObj(lh);
}
rs::LCValue InfoShow::MySt::recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) {
	return rs::LCValue();
}
// ---------------------- InfoShow ----------------------
InfoShow::InfoShow():
	_textHud(T_Info)
{
	//  フォント読み込み
	rs::CCoreID cid = mgr_text.makeCoreID("IPAGothic", rs::CCoreID(0, 20, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel));
	_textHud.setCCoreId(cid);
	_textHud.setDepth(0.f);

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
rs::Priority InfoShow::getPriority() const { return 0x1000; }
