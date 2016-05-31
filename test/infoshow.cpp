#include "test.hpp"
#include "infoshow.hpp"
#include "engine.hpp"
#include "../gpu.hpp"
#include "../gameloophelper.hpp"
#include "../differential.hpp"
#include "../systeminfo.hpp"

const std::string g_fontName("IPAGothic");
const rs::IdValue TextShow::T_Text = GlxId::GenTechId("Text", "Default");
const rs::IdValue T_Rect = GlxId::GenTechId("Sprite", "Rect");
// ---------------------- TextShow ----------------------
TextShow::TextShow(rs::Priority dprio) {
	//  フォント読み込み
	rs::CCoreID cid = mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, 20, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Pixel));
	_textHud.setCCoreId(cid);
	_textHud.setDepth(0.f);
	_dtag.priority = dprio;
	_dtag.idTechPass = T_Text;

	// 背景
	_rect.setAlpha(0.5f);
	_rect.setColor({0,1,0});
	_rect.setDepth(1.f);

	setStateNew<St_Default>();
}
void TextShow::setText(const std::string& str) {
	_textHud.setText(spn::Text::UTFConvertTo32(str));
	_textHud.exportDrawTag(_dtag);
	auto sz = _textHud.getText()->getSize();
	_rect.setScale({sz.width, -sz.height});
}
void TextShow::setOffset(const spn::Vec2& ofs) {
	_textHud.setWindowOffset(ofs);
	_rect.setOffset(ofs);
}
void TextShow::setBGDepth(float d) {
	_rect.setDepth(d);
}
void TextShow::setBGColor(const spn::Vec3& c) {
	_rect.setColor(c);
}
void TextShow::setBGAlpha(float a) {
	_rect.setAlpha(a);
}
rs::Priority TextShow::getPriority() const {
	return 0x1000; }

// ---------------------- TextShow::St_Default ----------------------
struct TextShow::St_Default : StateT<St_Default> {
	void onDraw(const TextShow& self, rs::IEffect& e) const override {
		// 背景
		e.setTechPassId(T_Rect);
		self._rect.draw(e);
		// 文字列
		e.setTechPassId(T_Text);
		self._textHud.draw(e);
	}
};

// ---------------------- InfoShow ----------------------
InfoShow::InfoShow(rs::HDGroup hDg, rs::Priority dprio):
	ObjectT(dprio),
	_hDg(hDg)
{
	rs::GPUInfo info;
	info.onDeviceReset();
	std::stringstream ss;
	ss << "Version: " << info.version() << std::endl
			<< "GLSL Version: " << info.glslVersion() << std::endl
			<< "Vendor: " << info.vendor() << std::endl
			<< "Renderer: " << info.renderer() << std::endl
			<< "DriverVersion: " << info.driverVersion() << std::endl;
	_baseText = ss.str();
	setStateNew<St_Default>();
}

const std::string MSG_GetState("GetState");
// ---------------------- InfoShow::MySt ----------------------
struct InfoShow::St_Default : StateT<St_Default> {
	void onConnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDisconnected(InfoShow& self, rs::HGroup hGroup) override;
	void onDraw(const InfoShow& self, rs::IEffect& e) const override;
	void onUpdate(InfoShow& self) override;
};
void InfoShow::St_Default::onDraw(const InfoShow& self, rs::IEffect& e) const {
	// FPSの表示
	int fps = mgr_info.getFPS();
	std::stringstream ss;
	ss << "FPS: " << fps << std::endl;
	ss << "State: " << self._stateName << std::endl;
	// バッファ切り替えカウント数の表示
	auto& buffd = self._count.buffer;
	ss << "VertexBuffer: " << buffd.vertex << std::endl;
	ss << "IndexBuffer: " << buffd.index << std::endl;
	ss << "DrawIndexed: " << self._count.drawIndexed << std::endl;
	ss << "DrawNoIndexed: " << self._count.drawNoIndexed << std::endl;
	auto& ths = const_cast<InfoShow&>(self);
	ths.setText(self._baseText+ ss.str());
	self.TextShow::onDraw(e);
}

#include "spinner/structure/profiler.hpp"
#include <thread>
void InfoShow::St_Default::onUpdate(InfoShow& self) {
	{
		auto lk = sharedbase.lockR();
		self._count = lk->diffCount;
	}
	// ステート名を取得, 保存しておく -> onDrawで使用
	rs::Object& obj = *mgr_scene.getScene(0).ref();
	if(auto ret = obj.recvMsgLua(MSG_GetState)) {
		auto& tbl = *boost::get<rs::SPLCTable>(ret);
		self._stateName = tbl[1].toString();
	}
}
void InfoShow::St_Default::onConnected(InfoShow& self, rs::HGroup) {
	auto* d = self._hDg->get();
	auto lh = rs::HDObj::FromHandle(self.handleFromThis());
	d->addObj(lh);
}
void InfoShow::St_Default::onDisconnected(InfoShow& self, rs::HGroup) {
	auto* d = self._hDg->get();
	auto lh = rs::HDObj::FromHandle(self.handleFromThis());
	d->remObj(lh);
}
