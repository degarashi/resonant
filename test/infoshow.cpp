#include "test.hpp"
#include "gpu.hpp"

// ---------------------- InfoShow::InfoDraw ----------------------
InfoShow::InfoDraw::InfoDraw(const InfoShow& is):
	_info(is) {}
void InfoShow::InfoDraw::onDraw() const {
	auto lk = shared.lock();
	auto& fx = *lk->pFx;
	fx.setTechnique(_info._techId, true);
	fx.setPass(_info._passId);

	fx.setPass(_info._passId);
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

	_hlText = mgr_text.createText(_info._charId, _info._infotext + spn::Text::UTFConvertTo32(ss.str()).c_str());
	_hlText.cref().draw(&fx);
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
}
void InfoShow::onConnected(rs::HGroup) {
	_hlDraw = rs_mgr_obj.makeDrawable<InfoDraw>(*this);

	auto& sb = mgr_scene.getSceneBase();
	sb.draw->get()->addObj(_hlDraw);
}
void InfoShow::onDisconnected(rs::HGroup) {
	PrintLog;
}

