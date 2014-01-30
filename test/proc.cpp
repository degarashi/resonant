#include "test.hpp"
#include "font.hpp"
#include "sound.hpp"
#include "glresource.hpp"
#include "gpu.hpp"
#include "camera.hpp"
#include "input.hpp"

void MyMain::_initInput() {
	auto lk = shared.lock();
	lk->hlIk = rs::Keyboard::OpenKeyboard();

	lk->actQuit = mgr_input.addAction("quit");
	mgr_input.link(lk->actQuit, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_ESCAPE));
	lk->actButton = mgr_input.addAction("button");
	mgr_input.link(lk->actButton, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_LSHIFT));

	lk->actLeft = mgr_input.addAction("left");
	lk->actRight = mgr_input.addAction("right");
	lk->actUp = mgr_input.addAction("up");
	lk->actDown = mgr_input.addAction("down");
	lk->actMoveX = mgr_input.addAction("moveX");
	lk->actMoveY = mgr_input.addAction("moveY");
	lk->actPress = mgr_input.addAction("press");
	mgr_input.link(lk->actLeft, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_A));
	mgr_input.link(lk->actRight, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_D));
	mgr_input.link(lk->actUp, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_W));
	mgr_input.link(lk->actDown, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_S));

	lk->hlIm = rs::Mouse::OpenMouse(0);
	lk->hlIm.ref()->setMouseMode(rs::MouseMode::Absolute);
	lk->hlIm.ref()->setDeadZone(0, 1.f, 0.f);
	lk->hlIm.ref()->setDeadZone(1, 1.f, 0.f);
	mgr_input.link(lk->actMoveX, rs::InF::AsAxis(lk->hlIm, 0));
	mgr_input.link(lk->actMoveY, rs::InF::AsAxis(lk->hlIm, 1));
	mgr_input.link(lk->actPress, rs::InF::AsButton(lk->hlIm, 0));

	_bPress = false;
}
void MyMain::_initDraw() {
	using spn::Vec3;
	using spn::Vec4;
	// 頂点定義
	struct TmpV {
		Vec3 pos;
		Vec4 tex;
	};
	TmpV tmpV[] = {
		{
			Vec3{-1,-1,0},
			Vec4{0,1,0,0}
		},
		{
			Vec3{-1,1,0},
			Vec4{0,0,0,0}
		},
		{
			Vec3{1,1,0},
			Vec4{1,0,0,0}
		},
		{
			Vec3{1,-1,0},
			Vec4{1,1,0,0}
		}
	};
	// インデックス定義
	GLubyte tmpI[] = {
		0,1,2,
		2,3,0
	};
	_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
	_hlVb.ref()->initData(tmpV, countof(tmpV), sizeof(TmpV));
	_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
	_hlIb.ref()->initData(tmpI, countof(tmpI));

	// テクスチャ
	spn::URI uriTex("file", mgr_path.getPath(rs::AppPath::Type::Texture));
	uriTex <<= "test.png";
	_hlTex = mgr_gl.loadTexture(uriTex);
	_hlTex.ref()->setFilter(rs::IGLTexture::MipmapLinear, true,true);

	// テキスト
	_charID = mgr_text.makeCoreID("MS Gothic", rs::CCoreID(0, 3, 0, false, 0, rs::CCoreID::SizeType_Point));

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
void MyMain::_initCam() {
	auto lk = shared.lock();
	lk->hlCam = mgr_cam.emplace();
	rs::CamData& cd = lk->hlCam.ref();
	cd.setOfs(0,0,-3);
	cd.setFOV(spn::DEGtoRAD(60));
	cd.setZPlane(0.01f, 500.f);
}
void MyMain::_initEffect() {
	auto lk = shared.lock();
	spn::URI uriFx("file", mgr_path.getPath(rs::AppPath::Type::Effect));
	uriFx <<= "test.glx";
	lk->hlFx = mgr_gl.loadEffect(uriFx);
	auto& pFx = *lk->hlFx.ref();
	_techID = pFx.getTechID("TheTech");
	pFx.setTechnique(_techID, true);

	_passView = pFx.getPassID("P0");
	_passText = pFx.getPassID("P1");
}

MyMain::MyMain(const rs::SPWindow& sp) {
	mgr_rw.addUriHandler(rs::SPUriHandler(new rs::UriH_File(u8"/")));
	_size *= 0;
	auto lk = shared.lock();
	lk->spWin = sp;

	_initInput();
	_initDraw();
	_initCam();
	_initEffect();

	mgr_scene.setPushScene(mgr_gobj.makeObj<TScene>());
}
bool MyMain::runU() {
	mgr_sound.update();
	if(mgr_scene.onUpdate())
		return false;

	auto lk = shared.lock();
	// 画面のサイズとアスペクト比合わせ
	auto& cd = lk->hlCam.ref();
	auto sz = lk->spWin->getSize();
	if(sz != _size) {
		_size = sz;
		cd.setAspect(float(_size.width)/_size.height);
		GL.glViewport(0,0,_size.width, _size.height);
	}
	// カメラ操作
	auto btn = mgr_input.isKeyPressing(lk->actPress);
	if(btn ^ _bPress) {
		lk->hlIm.ref()->setMouseMode((!_bPress) ? rs::MouseMode::Relative : rs::MouseMode::Absolute);
		_bPress = btn;
	}
	constexpr float speed = 0.25f;
	float mvF=0, mvS=0;
	if(mgr_input.isKeyPressing(lk->actUp))
		mvF += speed;
	if(mgr_input.isKeyPressing(lk->actDown))
		mvF -= speed;
	if(mgr_input.isKeyPressing(lk->actLeft))
		mvS -= speed;
	if(mgr_input.isKeyPressing(lk->actRight))
		mvS += speed;
	cd.moveFwd3D(mvF);
	cd.moveSide3D(mvS);
	if(_bPress) {
		float xv = mgr_input.getKeyValue(lk->actMoveX)/4.f,
			yv = mgr_input.getKeyValue(lk->actMoveY)/4.f;
		cd.addRot(spn::Quat::RotationY(spn::DEGtoRAD(-xv)));
		cd.addRot(spn::Quat::RotationX(spn::DEGtoRAD(-yv)));
	}

	// 描画コマンド
	auto& fx = *lk->hlFx.ref();
	fx.beginTask();

	fx.setTechnique(_techID, true);
	fx.setPass(_passView);
	fx.setUniform(fx.getUniformID("mTrans"), cd.getViewProjMatrix().convert44());
	fx.setUniform(fx.getUniformID("tDiffuse"), _hlTex);
	// 頂点フォーマット定義
	rs::SPVDecl decl(new rs::VDecl{
		{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
		{0,12, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::TEXCOORD0}
	});
	fx.setVDecl(std::move(decl));
	fx.setVStream(_hlVb.get(), 0);
	fx.setIStream(_hlIb.get());
	fx.drawIndexed(GL_TRIANGLES, 6, 0);

	fx.setPass(_passText);
	auto tsz = _size;
	auto fn = [tsz](int x, int y, float r) {
		float rx = spn::Rcp22Bit(tsz.width/2),
			ry = spn::Rcp22Bit(tsz.height/2);
		return spn::Mat33(rx*r,		0,			0,
						0,			ry*r, 		0,
						-1.f+x*rx,	1.f-y*ry,	1);
	};
	fx.setUniform(fx.getUniformID("mText"), fn(0,0,1));

	int fps = lk->fps.getFPS();
	std::stringstream ss;
	ss << "FPS: " << fps;
	_hlText = mgr_text.createText(_charID, _infotext + spn::Text::UTFConvertTo32(ss.str()).c_str());
	_hlText.ref().draw(&fx);

	return true;
}
void MyMain::onPause() {
	mgr_scene.onPause(); }
void MyMain::onResume() {
	mgr_scene.onResume(); }
void MyMain::onStop() {
	auto lk = shared.lock();
	lk->hlFx.ref()->clearTask();
	mgr_scene.onStop(); }
void MyMain::onReStart() {
	mgr_scene.onReStart(); }
