#include "gameloop.hpp"
#include "input.hpp"
#include "font.hpp"
#include "spinner/frac.hpp"
#include "camera.hpp"
#include "updater.hpp"
#include "scene.hpp"
#include "sound.hpp"
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include "serialization/chrono.hpp"
#include "serialization/chars.hpp"
#include "spinner/random.hpp"

namespace rs {
	// --------------------- FPSCounter ---------------------
	FPSCounter::FPSCounter() {
		reset();
	}
	void FPSCounter::reset() {
		_counter = _fps = 0;
		_tmBegin = Clock::now();
	}
	void FPSCounter::update() {
		Timepoint tp = Clock::now();
		Duration dur = tp - _tmBegin;
		if(dur >= std::chrono::seconds(1)) {
			_tmBegin = tp;
			_fps = _counter;
			_counter = 0;
		}
		++_counter;
	}
	int FPSCounter::getFPS() const { return _fps; }

	// --------------------- DrawThread ---------------------
	DrawThread::DrawThread(): ThreadL("DrawThread") {}
	void DrawThread::runL(const SPLooper& mainLooper, const SPWindow& w, IDrawProc* dproc) {
		Handler mainHandler(mainLooper);
		{	// msg::MakeContext と動作は同じ
			auto lk = _info.lock();
			lk->ctx = GLContext::CreateContext(w, false);
			lk->ctx->makeCurrent(w);
		}
		Handler drawHandler(Looper::GetLooper());
		GLW.initializeDrawThread(drawHandler);
		GLW.loadGLFunc();
		// ここで一旦MainThreadにOpenGLコンテキストの初期化が終ったことを通知
		mainHandler.postArgs(msg::DrawInit());
		UPDrawProc up(dproc);

		bool bLoop = true;
		do {
			// メインスレッドから描画開始の指示が来るまで待機
			// AndroidではContextSharingが出来ないのでメインスレッドからロードするタスクを受け取ってここで処理
			while(auto m = getLooper()->wait()) {
				GL.setSwapInterval(0);
				if(msg::DrawReq* p = *m) {
					_info.lock()->state = State::Drawing;
					// 1フレーム分の描画処理
					if(up->runU(p->id, p->bSkip))
						_info.lock()->ctx->swapWindow();
					GL.glFinish();
					{
						auto lk = _info.lock();
						lk->state = State::Idle;
						lk->accum = p->id;
					}
				} else if(static_cast<msg::QuitReq*>(*m)) {
					bLoop = false;
					break;
				} else if(static_cast<msg::MakeContext*>(*m)) {
					{
						auto lk = _info.lock();
						auto& ctx = lk->ctx;
						if(!ctx) {
	// 						Assert(Warn, !static_cast<bool>(lk->ctx), "initialized OpenGL context twice")
							// OpenGLコンテキストを再度作成
							ctx = GLContext::CreateContext(w, false);
							ctx->makeCurrent(w);
						}
					}
					// OpenGLリソースの再確保
					mgr_gl.onDeviceReset();
					GL.glFlush();
					GL.setSwapInterval(0);
				} else if(static_cast<msg::DestroyContext*>(*m)) {
					mgr_gl.onDeviceLost();
					GL.glFlush();
					auto lk = _info.lock();
					Assert(Warn, static_cast<bool>(lk->ctx), "OpenGL context is not initialized")
					lk->ctx.reset();
				}
			}
		} while(bLoop && !isInterrupted());
		std::cout << "DrawThread destructor begun" << std::endl;

		up.reset();
		// 後片付けフェーズ
		bLoop = true;
		while(bLoop && !isInterrupted()) {
			while(auto m = getLooper()->wait()) {
				if(static_cast<msg::QuitReq*>(*m)) {
					bLoop = false;
					break;
				}
			}
		}
		GLW.terminateDrawThread();
		std::cout << "DrawThread destructor ended" << std::endl;
	}

	namespace {
		constexpr int MAX_SKIPFRAME = 3;
		constexpr int DRAW_THRESHOLD_USEC = 2000;
	}
	// --------------------- IMainProc::Query ---------------------
	IMainProc::Query::Query(Timepoint tp, int skip):
		_bDraw(false), _tp(tp), _skip(skip) {}
	bool IMainProc::Query::canDraw() const {
		return true;
		using namespace std::chrono;
		auto dur = Clock::now() - _tp;
		return _skip >= MAX_SKIPFRAME || dur <= microseconds(DRAW_THRESHOLD_USEC);
	}
	void IMainProc::Query::setDraw(bool bDraw) {
		_bDraw = bDraw;
	}
	bool IMainProc::Query::getDraw() const {
		return _bDraw;
	}

	constexpr bool MULTICONTEXT = false;
	// --------------------- MainThread ---------------------
	MainThread::MainThread(MPCreate mcr, DPCreate dcr): ThreadL("MainThread"), _mcr(mcr), _dcr(dcr) {
		auto lk = _info.lock();
		lk->accumUpd = lk->accumDraw = 0;
		lk->tmBegin = Clock::now();
	}
	void MainThread::runL(const SPLooper& guiLooper, const SPWindow& w, const GLoopInitParam& param) {
		spn::Optional<DrawThread> opDth;
		UPtr<GLWrap>	glw;
		// Looper::atPanic
		try {
			glw.reset(new GLWrap(MULTICONTEXT));
			// 描画スレッドを先に初期化
			opDth = spn::construct();
			opDth->start(std::ref(getLooper()), w, _dcr());
			// 描画スレッドの初期化完了を待つ
			while(auto m = getLooper()->wait()) {
				if(static_cast<msg::DrawInit*>(*m))
					break;
			}
			GLW.initializeMainThread();

			SPGLContext ctx;
			auto fnMakeContext = [&opDth, &ctx, &w](){
				// ポーリングでコンテキスト初期化の完了を検知
				for(;;) {
					if(opDth->getInfo()->ctx)
						break;
					SDL_Delay(0);
				}
				if(MULTICONTEXT) {
					ctx = GLContext::CreateContext(w, true);
					ctx->makeCurrent(w);
				}
			};
			fnMakeContext();

			UPtr<spn::MTRandomMgr>	randP(new spn::MTRandomMgr());
			UPtr<GLRes>			glrP(new GLRes());
			UPtr<RWMgr>			rwP(new RWMgr(param.organization, param.app_name));
			UPtr<AppPath>		appPath(new AppPath(spn::Dir::GetProgramDir().c_str()));
			appPath->setFromText(mgr_rw.fromFile(param.pathfile, RWops::Read));
			UPtr<FontFamily>	fontP(new FontFamily());
			fontP->loadFamilyWildCard(mgr_path.getPath(AppPath::Type::Font).plain_utf8());
			UPtr<FontGen>		fgenP(new FontGen(spn::PowSize(512,512)));
			UPtr<CameraMgr>		camP(new CameraMgr());
			UPtr<PointerMgr>	pmP(new PointerMgr());
			UPtr<InputMgr>		inpP(new InputMgr());
			UPtr<ObjMgr>		objP(new ObjMgr());
			UPtr<UpdMgr>		updP(new UpdMgr());
			UPtr<SceneMgr>		scP(new SceneMgr());
			UPtr<SoundMgr>		sndP(new SoundMgr(44100));
			UPtr<UpdRep>		urep(new UpdRep());
			UPtr<ObjRep>		orep(new ObjRep());
			sndP->makeCurrent();
			using UPHandler = UPtr<Handler>;
			UPHandler drawHandler(new Handler(opDth->getLooper()));
			drawHandler->postArgs(msg::MakeContext());

			// 続いてメインスレッドを初期化
			UPMainProc mp(_mcr(w));
			UPHandler guiHandler(new Handler(guiLooper, [](){
				// メッセージを受け取る度にSDLイベントを発生させる
				SDL_Event e;
				e.type = EVID_SIGNAL;
				SDL_PushEvent(&e);
			}));
			guiHandler->postArgs(msg::MainInit());

			const spn::FracI fracInterval(50000, 3);
			spn::FracI frac(0,1);
			Timepoint prevtime = Clock::now();
			int skip = 0;
PrintLog;

			using namespace std::chrono;
			// ゲームの進行や更新タイミングを図って描画など
			bool bLoop = true;
			do {
				if(!opDth->isRunning()) {
					// 何らかの原因で描画スレッドが終了していた時
					try {
						// 例外が投げられて終了したかをチェック
						opDth->getResult();
					} catch (...) {
						guiHandler->postArgs(msg::QuitReq());
						Assert(Warn, false, "MainThread: draw thread was ended by throwing exception")
						throw;
					}
					Assert(Warn, false, "MainThread: draw thread was ended unexpectedly")
					break;
				}
				// 何かメッセージが来てたら処理する
				while(OPMessage m = getLooper()->peek(std::chrono::seconds(0))) {
					if(static_cast<msg::PauseReq*>(*m)) {
						mgr_sound.pauseAllSound();
						// ユーザーに通知(Pause)
						mp->onPause();
						std::stringstream buffer;	// サウンドデバイスデータ
						for(;;) {
							// DrawThreadがIdleになるまで待つ
							while(opDth->getInfo()->accum != getInfo()->accumDraw)
								SDL_Delay(0);

							// Resumeメッセージが来るまでwaitループ
							OPMessage m = getLooper()->wait();
							if(static_cast<msg::ResumeReq*>(*m)) {
								mgr_sound.resumeAllSound();
								// ユーザーに通知(Resume)
								mp->onResume();
								break;
							} else if(static_cast<msg::StopReq*>(*m)) {
								mp->onStop();
								// OpenGLリソースの解放をDrawThreadで行う
								drawHandler->postArgs(msg::DestroyContext());
								// サウンドデバイスのバックアップ
								boost::archive::binary_oarchive oa(buffer);
	// 							oa << mgr_rw;
	//							mgr_rw.resetSerializeFlag();
								SoundMgr* sp = sndP.get();
								oa << sp;
								sp->resetSerializeFlag();
								sp->invalidate();
								// サウンドを一旦全部停止させておく -> 復元した時に以前再生していたものは処理が継続される
								sndP.reset(nullptr);
							}
							else if(static_cast<msg::ReStartReq*>(*m)) {
								// サウンドデバイスの復元
								boost::archive::binary_iarchive ia(buffer);
	//							ia >> mgr_rw;
								SoundMgr* sp = nullptr;
								ia >> sp;
								sndP.reset(sp);
								sndP->makeCurrent();
								std::cout << "------------";
								sndP->update();
								// DrawThreadでOpenGLの復帰処理を行う
								drawHandler->postArgs(msg::MakeContext());
								fnMakeContext();
								mp->onReStart();
							}
						}
					}
				}
				// 次のフレーム開始を待つ
				auto ntp = prevtime + microseconds(16666);
				auto tp = Clock::now();
				if(ntp <= tp)
					ntp = tp;
				else {
					auto dur = ntp - tp;
					if(dur >= microseconds(1000)) {
						// 時間に余裕があるならスリープをかける
						SDLEC_D(Warn, SDL_Delay, duration_cast<milliseconds>(dur).count() - 1);
					}
					// スピンウェイト
					while(Clock::now() < ntp);
				}
				prevtime = ntp;

				// ゲーム進行
				++getInfo()->accumUpd;
				mgr_input.update();
PrintLog;
				IMainProc::Query q(tp, skip);
				if(!mp->runU(q)) {
					PrintLogMsg("MainLoop END");
					break;
				}
PrintLog;
				// 時間が残っていれば描画
				// 最大スキップフレームを超過してたら必ず描画
				bool bSkip = !q.getDraw();
				if(!bSkip)
					skip = 0;
				else
					++skip;
				drawHandler->postArgs(msg::DrawReq(++getInfo()->accumDraw, bSkip));
			} while(bLoop && !isInterrupted());
			while(mgr_scene.getTop().valid()) {
				mgr_scene.setPopScene(1);
				mgr_scene.onUpdate();
			}
			// DrawThreadがIdleになるまで待つ
			while(opDth->getInfo()->accum != getInfo()->accumDraw)
				SDL_Delay(0);

			// 描画スレッドを後片付けフェーズへ移行
			drawHandler->postArgs(msg::QuitReq());

			mp.reset();

			orep.reset();
			urep.reset();
			sndP.reset();
			scP.reset();
			updP.reset();
			objP.reset();
			inpP.reset();
			camP.reset();
			fgenP.reset();
			fontP.reset();
			appPath.reset();
			rwP.reset();
			glrP.reset();
		} catch (const std::exception& e) {
			LogOutput("MainThread: exception\n%s", e.what());
		} catch (...) {
			LogOutput("MainThread: unknown exception");
		}
		// 描画スレッドの終了を待つ
		if(opDth) {
			opDth->interrupt();
			opDth->getLooper()->pushEvent(Message(Seconds(0), msg::QuitReq()));
			opDth->join();
			guiLooper->pushEvent(Message(Seconds(0), msg::QuitReq()));
		}
		LogOutput("MainThread ended");
	}

	void GameLoop::_procWindowEvent(SDL_Event& e) {
		switch(e.window.event) {
			case SDL_WINDOWEVENT_CLOSE:
				// ウィンドウ閉じたら終了
				e.type = SDL_QUIT;
				e.quit.timestamp = 0;
				SDL_PushEvent(&e);
				break;
			case SDL_WINDOWEVENT_MINIMIZED:
				_setLevel(std::min(_level, Stop));
				break;
			case SDL_WINDOWEVENT_RESTORED:
				_setLevel(std::max(_level, Pause));
				break;
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				_setLevel(std::max(_level, Active));
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				_setLevel(std::min(_level, Pause));
				break;
			default: break;
		}
	}

	// それぞれユーザースレッドに通知
	void GameLoop::_onPause() {
		_handler->postArgs(msg::PauseReq());
	}
	void GameLoop::_onResume() {
		_handler->postArgs(msg::ResumeReq());
	}
	void GameLoop::_onStop() {
		_handler->postArgs(msg::StopReq());
	}
	void GameLoop::_onReStart() {
		_handler->postArgs(msg::ReStartReq());
	}
	void GameLoop::_setLevel(Level level) {
		int ilevel = level,
			curLevel = _level;
		int idx = ilevel > curLevel ? 0 : 1,
			inc = ilevel > curLevel ? 1 : -1;
		while(ilevel != curLevel) {
			if(LFunc f = cs_lfunc[curLevel][idx])
				(this->*f)();
			curLevel += inc;
		}
		_level = level;
	}
	const GameLoop::LFunc GameLoop::cs_lfunc[NumLevel][2] = {
		{&GameLoop::_onReStart, nullptr},
		{&GameLoop::_onResume, &GameLoop::_onStop},
		{nullptr, &GameLoop::_onPause}
	};
	const uint32_t EVID_SIGNAL = SDL_RegisterEvents(1);
	GameLoop::GameLoop(MPCreate mcr, DPCreate dcr): _mcr(mcr), _dcr(dcr), _level(Active) {}
	int GameLoop::run(const GLoopInitParam& param) {
		SDLInitializer	sdlI(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
		IMGInitializer imgI(IMG_INIT_JPG | IMG_INIT_PNG);

		auto logOld = g_logOut;
		g_logOut = [logOld](const std::string& s) {
			// スレッド番号を出力
			boost::format msg("thread=%1%, %2%");
			if(tls_threadName.initialized())
				logOld((msg % *tls_threadName % s).str());
			else {
				auto thId = SDL_GetThreadID(nullptr);
				logOld((msg % thId % s).str());
			}
		};
		tls_threadID = SDL_GetThreadID(nullptr);
		tls_threadName = "GuiThread";
		#ifdef ANDROID
			// egl関数がロードされてないとのエラーが出る為
			SDL_GL_LoadLibrary("libEGL.so");
		#endif
		param.gparam.setStdAttributes();
		SPWindow _spWindow = Window::Create(param.wparam);
		rs::SDLMouse::SetWindow(_spWindow->getWindow());

		// メインスレッドのメッセージキューを初期化
		Looper::Prepare();
		auto& loop = Looper::GetLooper();
		// メインスレッドに渡す
		MainThread mth(_mcr, _dcr);
		mth.start(std::ref(loop), std::ref(_spWindow), param);
		// メインスレッドのキューが準備出来るのを待つ
		while(auto msg = loop->wait()) {
			if(static_cast<msg::MainInit*>(*msg))
				break;
			else if(static_cast<msg::QuitReq*>(*msg))
				break;
		}
		_handler = Handler(mth.getLooper());
		// GUIスレッドのメッセージループ
		SDL_Event e;
		bool bLoop = true;
		while(bLoop && mth.isRunning() && SDL_WaitEvent(&e)) {
			if(e.type == EVID_SIGNAL) {
				// 自作スレッドのキューにメッセージがある
				while(OPMessage m = loop->peek(std::chrono::seconds(0))) {
					if(static_cast<msg::QuitReq*>(*m)) {
						e.type = SDL_QUIT;
						e.quit.timestamp = 0;
						SDL_PushEvent(&e);
					}
				}
			} else {
				PrintEvent::All(e);
				// 当分はGame_Sleep()でリソース解放、Game_Wakeup()でリソース復帰とする
				// (onStop()やonStart()は関知しない)
				switch(e.type) {
					case SDL_WINDOWEVENT:
						_procWindowEvent(e); break;
					case SDL_QUIT:
						// アプリケーション終了コールが来たらループを抜ける
						bLoop = false;
						break;
					case SDL_APP_TERMINATING:			// Android: onDestroy()
						break;
					case SDL_APP_LOWMEMORY:				// Android: onLowMemory()
						break;
					case SDL_APP_WILLENTERBACKGROUND:	// Android: onPause()
					case SDL_APP_DIDENTERBACKGROUND:	// Android: onPause()
						_setLevel(std::min(_level, Stop));
						break;
					case SDL_APP_WILLENTERFOREGROUND:	// Android: onResume()
					case SDL_APP_DIDENTERFOREGROUND:	// Android: onResume()
						_setLevel(std::max(_level, Active));
						break;
				}
			}
		}
		mth.interrupt();
		try {
			// 例外が投げられて終了したかをチェック
			mth.getResult();
		} catch (...) {
			Assert(Warn, false, "GuiThread: main thread was ended by throwing exception")
		}
		return 0;
	}
}

