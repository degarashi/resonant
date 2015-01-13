#include "test.hpp"
#include "gpu.hpp"

// ---------------------- InfoShow::MySt ----------------------
void InfoShow::MySt::onDraw(const InfoShow& self) const {
	auto lk = shared.lock();
	auto& fx = *lk->pFx;
	fx.setTechnique(self._techId, true);
	fx.setPass(self._passId);

	fx.setPass(self._passId);
	auto lkb = sharedbase.lock();
	auto tsz = lkb->screenSize;
	auto fn = [tsz](int x, int y, float r) {
		float rx = spn::Rcp22Bit(tsz.width/2),
			  ry = spn::Rcp22Bit(tsz.height/2);
		return spn::Mat33(rx*r,		0,			0,
						0,			ry*r, 		0,
						-1.f+x*rx,	1.f-y*ry,	1);
	};
	fx.setUniform(*fx.getUniformID("mText"), fn(0,0,1), true);

	int fps = lkb->fps.getFPS();
	std::stringstream ss;
	ss << "FPS: " << fps << std::endl;
	rs::Object& obj = *mgr_scene.getScene(0).ref();
	auto var = obj.recvMsg(MSG_GetStatus);
	ss << "Status: " << var.toCStr() << std::endl;

	self._hlText = mgr_text.createText(self._charId, self._infotext + spn::Text::UTFConvertTo32(ss.str()).c_str());
	self._hlText.cref().draw(&fx);
}
void InfoShow::MySt::onConnected(InfoShow& self, rs::HGroup) {
	auto lh = self.handleFromThis();
	auto& d = mgr_scene.getSceneBase().draw;
	d->get()->addObj(lh);
}
void InfoShow::MySt::onDisconnected(InfoShow& self, rs::HGroup) {
	auto lh = self.handleFromThis();
	auto& d = mgr_scene.getSceneBase().draw;
	d->get()->remObj(lh);
}
rs::LCValue InfoShow::MySt::recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) {
	return rs::LCValue();
}

// ---------------------- InfoShow ----------------------
InfoShow::InfoShow(GLint techId, GLint passId):
	_techId(techId),
	_passId(passId)
{
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

