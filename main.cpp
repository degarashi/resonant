#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "gameloop.hpp"
#include "glhead.hpp"
#include "input.hpp"
#include "glresource.hpp"
#include "spinner/vector.hpp"
#include "gpu.hpp"
#include "glx.hpp"
#include "camera.hpp"
#include "font.hpp"
#include "updator.hpp"
#include "scene.hpp"
#include "sound.hpp"

using namespace rs;
using namespace spn;
// MainThread と DrawThread 間のデータ置き場
// リソースハンドルはメインスレッド
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
	rs::HLText	_hlText;

	spn::Size	_size;
	bool		_bPress;
	GLint		_passView, _passText;
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
			_hlTex = mgr_gl.loadTexture(spn::URI("file://Z:/home/slice/test.png"));
			_hlTex.ref()->use()->setFilter(rs::IGLTexture::MipmapLinear, true,true);

			rs::CCoreID cid = mgr_text.makeCoreID("MS Gothic", rs::CCoreID(0, 15, CCoreID::CharFlag_AA, false, 1, CCoreID::SizeType_Point));
			_hlText = mgr_text.createText(cid, U"おお_ゆうしゃよ\nなんということじゃ\nつるぎの　もちかたが　ちがうぞ");

			rs::GPUInfo info;
			info.onDeviceReset();
			std::cout << info;

			_hlFx = mgr_gl.loadEffect(spn::URI("file://Z:/home/slice/test.glx"));
			auto& pFx = *_hlFx.ref();
			GLint techID = pFx.getTechID("TheTech");
			pFx.setTechnique(techID, true);

			_passView = pFx.getPassID("P0");
			_passText = pFx.getPassID("P1");
// 			pFx.setUniform(spn::Vec4{1,2,3,4}, pFx.getUniformID("lowVal"));
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
			auto& fx = *_hlFx.ref();
			fx.setPass(_passView);
			GLint id = fx.getUniformID("mTrans");

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

			fx.setUniform(cd.getViewProjMatrix().convert44(), id);
			id = fx.getUniformID("tDiffuse");
			fx.setUniform(_hlTex, id);

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
			id = fx.getUniformID("mText");

			auto fn = [](int x, int y, float r) {
				float rx = Rcp22Bit(512),
					ry = Rcp22Bit(384);
				return Mat33(rx*r,		0,			0,
							0,			ry*r, 		0,
							-1.f+x*rx,	1.f-y*ry,	1);
			};
			fx.setUniform(fn(0,0,1), id);
 			_hlText.ref().draw(&fx);
			return true;
		}
};
class TestObj : public ObjectT<TestObj> {
	class MySt : public State {
		public:
			MySt(StateID id): State(id) {}
			void onUpdate(TestObj& self) {
// 				self.destroy();
			}
	};
	public:
		TestObj() {
			LogOutput("TestObj::ctor");
			setStateNew<MySt>(128);
		}
		~TestObj() {
			LogOutput("TestObj::dtor");
		}
		void onDestroy() override {
			LogOutput("TestObj::onDestroy");
		}
};
class TScene2 : public Scene<TScene2> {
	class MySt : public State {
		public:
			MySt(): State(0) {}
			void onUpdate(TScene2& self) override {
				auto lk = shared.lock();
				if(mgr_input.isKeyPressed(lk->actRight))
					mgr_scene.setPopScene(1);
			}
	};
	public:
		TScene2() {
			setStateNew<MySt>();
			LogOutput("TScene2::ctor");
		}
		~TScene2() {
			LogOutput("TScene2::dtor");
		}
};
class TScene : public Scene<TScene> {
	HLAb _hlAb;
	HLSg _hlSg;
	class MySt : public State {
		public:
			MySt(StateID id): State(id) {}
			void onEnter(TScene& self, StateID prevID) override {
				self._hlAb = mgr_sound.loadOggStream(mgr_rw.fromFile("Z:/home/slice/test.ogg", RWops::Read, false));
				self._hlSg = mgr_sound.createSourceGroup(1);
			}
			void onUpdate(TScene& self) override {
				HGbj hGbj(rep_gobj.getObj(c_id));
				if(!hGbj.valid())
					self.destroy();
				auto lk = shared.lock();
				if(mgr_input.isKeyPressed(lk->actLeft))
					mgr_scene.setPushScene(mgr_gobj.emplace(new TScene2()));
				if(mgr_input.isKeyPressed(lk->actQuit))
					mgr_scene.setPopScene(1);
			}
			void onDown(TScene& self, ObjTypeID prevID, const Variant& arg) override {
				LogOutput("TScene::onDown");
				auto& s = self._hlSg.ref();
				s.clear();
				s.play(self._hlAb, 0);
			}
			void onPause(TScene& self) override {
				LogOutput("TScene::onPause");
			}
			void onResume(TScene& self) override {
				LogOutput("TScene::onResume");
			}
			void onStop(TScene& self) override {
				LogOutput("TScene::onStop");
			}
			void onReStart(TScene& self) override {
				LogOutput("TScene::onReStart");
			}
	};
	public:
		const static ObjID c_id;
		TScene(): Scene(0) {
			setStateNew<MySt>(256);
			LogOutput("TScene::ctor");
			HLGbj hg = mgr_gobj.emplace(new TestObj());
			rep_gobj.setObj(c_id, hg.weak());
			_update.addObj(0, hg);
			hg.release();
		}
		void onDestroy() override {
			LogOutput("TScene::onDestroy");
		}
		~TScene() {
			LogOutput("TScene::dtor");
		}
};
const ObjID TScene::c_id = rep_gobj.RegID("Player");

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

			lk->hlCam = mgr_cam.emplace();
			rs::CamData& cd = lk->hlCam.ref();
			cd.setOfs(0,0,-3);

			mgr_scene.setPushScene(mgr_gobj.emplace(new TScene()));
		}
		bool runU() override {
			mgr_sound.update();
			return !mgr_scene.onUpdate();
		}
		void onPause() override {
			mgr_scene.onPause(); }
		void onResume() override {
			mgr_scene.onResume(); }
		void onStop() override {
			mgr_scene.onStop(); }
		void onReStart() override {
			mgr_scene.onReStart(); }
		rs::IDrawProc* initDraw() override {
			return new MyDraw;
		}
};
int main(int argc, char **argv) {
	GameLoop gloop([](const rs::SPWindow& sp){ return new MyMain(sp); });
	return gloop.run("HelloSDL2", 1024, 768, SDL_WINDOW_SHOWN, 2,0,24);
}
