#include "gameloop.hpp"
#include "glhead.hpp"
#include "input.hpp"
#include "glresource.hpp"
#include "spinner/vector.hpp"
#include "gpu.hpp"
#include "glx.hpp"
#include "camera.hpp"

using namespace rs;
using namespace spn;
// MainThread と DrawThread 間のデータ置き場
struct Mth_DthData {
	rs::HLInput hlIk,
				hlIm;
	rs::HLAct actQuit,
			actButton,
			actLeft,
			actRight,
			actUp,
			actDown,
			actMoveX,
			actMoveY,
			actPress;
	rs::HLCam hlCam;
	rs::SPWindow spWin;
};
#define shared (Mth_Dth::_ref())
class Mth_Dth : public spn::Singleton<Mth_Dth>, public rs::SpinLock<Mth_DthData> {};

class MyDraw : public rs::IDrawProc {
	rs::HLFx	_hlFx;
	rs::HLVb 	_hlVb;
	rs::HLIb 	_hlIb;
	rs::HLTex 	_hlTex;
	spn::Size	_size;
	bool		_bPress;
	public:
		MyDraw() {
			_bPress = false;
			struct TmpV {
				spn::Vec3 pos;
				spn::Vec4 tex;
			};
			TmpV tmpV[] = {
				{
					spn::Vec3{-1,-1,0},
					spn::Vec4{0,1,0,0}
				},
				{
					spn::Vec3{-1,1,0},
					spn::Vec4{0,0,0,0}
				},
				{
					spn::Vec3{1,1,0},
					spn::Vec4{1,0,0,0}
				},
				{
					spn::Vec3{1,-1,0},
					spn::Vec4{1,1,0,0}
				}
			};
			// インデックス定義
			GLubyte tmpI[] = {
				0,1,2,
				2,3,0
			};
			mgr_rw.addUriHandler(rs::SPUriHandler(new rs::UriH_File(u8"/")));
			_hlVb = mgr_gl.makeVBuffer(GL_STATIC_DRAW);
			_hlVb.ref()->use()->initData(tmpV, countof(tmpV), sizeof(TmpV));
			_hlIb = mgr_gl.makeIBuffer(GL_STATIC_DRAW);
			_hlIb.ref()->use()->initData(tmpI, countof(tmpI));
			_hlTex = mgr_gl.loadTexture(spn::URI("file:///home/slice/test.png"));
			_hlTex.ref()->use()->setFilter(rs::IGLTexture::MipmapLinear, true,true);

			rs::GPUInfo info;
			info.onDeviceReset();
			std::cout << info;

			_hlFx = mgr_gl.loadEffect(spn::URI("file:///home/slice/test.glx"));
			auto& pFx = *_hlFx.ref();
			GLint techID = pFx.getTechID("TheTech");
			pFx.setTechnique(techID, true);
			GLint passID = pFx.getPassID("P0");
			pFx.setPass(passID);
			// 頂点フォーマット定義
			rs::SPVDecl decl(new rs::VDecl{
				{0,0, GL_FLOAT, GL_FALSE, 3, (GLuint)rs::VSem::POSITION},
				{0,12, GL_FLOAT, GL_FALSE, 4, (GLuint)rs::VSem::TEXCOORD0}
			});
			pFx.setVDecl(std::move(decl));
			pFx.setUniform(spn::Vec4{1,2,3,4}, pFx.getUniformID("lowVal"));

			_hlTex.ref()->save("/tmp/test.png");
			rs::SetSwapInterval(1);

			_size *= 0;
			auto lk = shared.lock();
			auto& cd = lk->hlCam.ref();
			cd.setFOV(spn::DEGtoRAD(60));
			cd.setZPlane(0.01f, 500.f);
		}
		bool runU(uint64_t accum) override {
			glClearColor(0,0,1,1);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

			auto lk = shared.lock();
			auto& cd = lk->hlCam.ref();
			auto sz = lk->spWin->getSize();
			if(sz != _size) {
				_size = sz;
				cd.setAspect(float(_size.width)/_size.height);
				glViewport(0,0,_size.width, _size.height);
			}
			auto* pFx = _hlFx.ref().get();
			GLint id = pFx->getUniformID("mTrans");

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

			pFx->setUniform(cd.getViewProjMatrix().convert44(), id);
			id = pFx->getUniformID("tDiffuse");
			pFx->setUniform(_hlTex, id);

			pFx->setVStream(_hlVb.get(), 0);
			pFx->setIStream(_hlIb.get());
			pFx->drawIndexed(GL_TRIANGLES, 6, 0);
			return true;
		}
};
class MyMain : public rs::IMainProc {
	Mth_Dth _mth;
	public:
		MyMain(const rs::SPWindow& sp) {
			auto lk = shared.lock();
			lk->hlIk = rs::Keyboard::OpenKeyboard();
			lk->spWin = sp;

			lk->actQuit = mgr_input.addAction("quit");
			mgr_input.link(lk->actQuit, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_ESCAPE));
			lk->actButton = mgr_input.addAction("button");
			mgr_input.link(lk->actQuit, rs::InF::AsButton(lk->hlIk, SDL_SCANCODE_LSHIFT));

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

			lk->hlCam = mgr_cam.emplace();
			rs::CamData& cd = lk->hlCam.ref();
			cd.setOfs(0,0,-3);
		}
		bool runU() override {
			auto lk = shared.lock();
			if(mgr_input.isKeyPressed(lk->actQuit))
				return false;
			return true;
		}
		void onPause() override {
			LogOutput("OnPause");
		}
		void onResume() override {
			LogOutput("OnResume");
		}
		rs::IDrawProc* initDraw() override {
			return new MyDraw;
		}
};
using namespace rs;
int main(int argc, char **argv) {
	GameLoop gloop([](const rs::SPWindow& sp){ return new MyMain(sp); });
	return gloop.run("HelloSDL2", 1024, 768, SDL_WINDOW_SHOWN, 2,0,24);
}
