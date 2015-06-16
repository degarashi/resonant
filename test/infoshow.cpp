#include "test.hpp"
#include "infoshow.hpp"
#include "../gpu.hpp"
#include "../gameloophelper.hpp"
#include "../differential.hpp"
#include "engine.hpp"

const rs::IdValue InfoShow::T_Info = GlxId::GenTechId("TheTech", "P1"),
				InfoShow::U_Text = GlxId::GenUnifId("mText");
struct InfoShow::MySt : StateT<MySt> {
	rs::LCValue recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) override;
	void onConnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDisconnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDraw(const InfoShow& self, rs::GLEffect& e) const override;
	void onUpdate(InfoShow& self) override;
};

// ---------------------- InfoShow::MySt ----------------------
void InfoShow::MySt::onDraw(const InfoShow& self, rs::GLEffect& e) const {
	auto& fx = static_cast<Engine&>(e);
	fx.setTechPassId(T_Info);
	auto lkb = sharedbase.lock();
	auto tsz = lkb->screenSize;
	auto fn = [tsz](int x, int y, float r) {
		float rx = spn::Rcp22Bit(tsz.width/2),
			  ry = spn::Rcp22Bit(tsz.height/2);
		return spn::Mat33(rx*r,		0,			0,
						0,			ry*r, 		0,
						-1.f+x*rx,	1.f-y*ry,	1);
	};
	fx.setUniform(U_Text, fn(0,0,1), true);

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

	self._hlText = mgr_text.createText(self._charId, self._infotext + spn::Text::UTFConvertTo32(ss.str()).c_str());
	self._hlText.cref().draw(&fx);
}
#include "../spinner/structure/profiler.hpp"
#include <thread>
void InfoShow::MySt::onUpdate(InfoShow& self) {
	auto lk = sharedbase.lock();
	self._count = lk->diffCount;

	if(self._hlText)
		self._hlText.cref().exportDrawTag(self._dtag);
}
void InfoShow::MySt::onConnected(InfoShow& self, rs::HGroup) {
	self._dtag.zOffset = 0.f;
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
InfoShow::InfoShow() {
	_dtag.zOffset = 0;
	_dtag.idTechPass = T_Info;
	//  フォント読み込み
	_charId = mgr_text.makeCoreID("IPAGothic", rs::CCoreID(0, 5, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Point));

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
