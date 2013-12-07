#include "gameloop.hpp"
#include "input.hpp"
#include "font.hpp"
#include "spinner/frac.hpp"
#include "camera.hpp"
#include "updator.hpp"
#include "scene.hpp"

namespace rs {
	// --------------------- DrawThread ---------------------
	void DrawThread::runL(const SPLooper& mainLooper, SPGLContext ctx_b, const SPWindow& w, const UPMainProc& mp) {
		Handler drawHandler(mainLooper);
		drawHandler.postArgs(msg::DrawInit());

		SPGLContext ctx(std::move(ctx_b));
		ctx->makeCurrent(w);
		LoadGLFunc();
		mgr_gl.onDeviceReset();
		UPDrawProc up(mp->initDraw());

		bool bLoop = true;
		do {
			// メインスレッドから描画開始の指示が来るまで待機
			while(auto m = getLooper()->wait()) {
				if(msg::DrawReq* p = *m) {
					_info.lock()->state = State::Drawing;
					// 1フレーム分の描画処理
					if(up->runU(p->id))
						ctx->swapWindow();
					glFinish();
					{
						auto lk = _info.lock();
						lk->state = State::Idle;
						lk->accum = p->id;
					}
				}
				// AndroidではContextSharingが出来ないのでメインスレッドからロードするタスクを受け取ってここで処理
			}
		} while(bLoop && !isInterrupted());
	}

	// --------------------- MainThread ---------------------
	MainThread::MainThread(MPCreate mcr): _mcr(mcr) {
		auto lk = _info.lock();
		lk->accumUpd = lk->accumDraw = 0;
		lk->tmBegin = Clock::now();
		lk->fps = 0;
	}
	void MainThread::runL(const SPLooper& guiLooper, const SPWindow& w) {
		GLRes 		glrP;
		RWMgr 		rwP;
		FontFamily	fontP;
		fontP.loadFamilyWildCard("/home/slice/.fonts/*.ttc");
		FontGen		fgenP(spn::PowSize(512,512));
		CameraMgr	camP;
		InputMgr	inpP;
		ObjMgr		objP;
		ObjRep		objrP;
		UpdMgr		updP;
		UpdRep		updrP;
		SceneMgr	scP;

		UPMainProc mp(_mcr(w));
		Handler guiHandler(guiLooper, [](){
			SDL_Event e;
			e.type = EVID_SIGNAL;
			SDL_PushEvent(&e);
		});

		SPGLContext ctx = GLContext::CreateContext(w, false),
					ctxD = GLContext::CreateContext(w, true);
		ctxD->makeCurrent();
		ctx->makeCurrent(w);
		DrawThread dth;
		dth.start(std::ref(getLooper()), std::move(ctxD), w, mp);
		// 描画スレッドの初期化完了を待つ
		while(auto m = getLooper()->wait()) {
			if(msg::DrawInit* p = *m)
				break;
		}
		Handler drawHandler(dth.getLooper());
		guiHandler.postArgs(msg::MainInit());

		const spn::FracI fracInterval(50000, 3);
		spn::FracI frac(0,1);
		Timepoint prevtime = Clock::now();
		int skip = 0;
		constexpr int MAX_SKIPFRAME = 3;
		constexpr int DRAW_THRESHOLD_USEC = 2000;

		using namespace std::chrono;
		// ゲームの進行や更新タイミングを図って描画など
		bool bLoop = true;
		do {
			if(!dth.isRunning()) {
				// 何らかの原因で描画スレッドが終了していた時
				try {
					// 例外が投げられて終了したかをチェック
					dth.getResult();
				} catch (...) {
					guiHandler.postArgs(msg::QuitReq());
					Assert(Warn, false, "MainThread: draw thread was ended by throwing exception")
					throw;
				}
				Assert(Warn, false, "MainThread: draw thread was ended unexpectedly")
				break;
			}
			// 何かメッセージが来てたら処理する
			while(OPMessage m = getLooper()->peek(std::chrono::seconds(0))) {
				if(msg::PauseReq* pr = *m) {
					// ユーザーに通知(Pause)
					mp->onPause();
					for(;;) {
						// DrawThreadがIdleになるまで待つ
						while(dth.getInfo()->accum != getInfo()->accumDraw)
							SDL_Delay(0);

						// Resumeメッセージが来るまでwaitループ
						OPMessage m = getLooper()->wait();
						if(msg::ResumeReq* rr = *m) {
							// ユーザーに通知(Resume)
							mp->onResume();
							break;
						} else if(msg::StopReq* sr = *m) {
							mp->onStop();
							// OpenGLリソースの解放
							mgr_gl.onDeviceLost();
							glFinish();
						}
						else if(msg::ReStartReq* rr = *m) {
							// OpenGLリソースの再確保
							mgr_gl.onDeviceReset();
							glFinish();
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
					SDLEC_P(Warn, SDL_Delay, duration_cast<milliseconds>(dur).count() - 1);
				}
				// スピンウェイト
				while(Clock::now() < ntp);
			}
			prevtime = ntp;

			// ゲーム進行
			++getInfo()->accumUpd;
			mgr_input.update();
			if(!mp->runU())
				break;

			// 時間が残っていれば描画
			// 最大スキップフレームを超過してたら必ず描画
			auto dur = Clock::now() - tp;
			auto count = std::chrono::duration_cast<std::chrono::microseconds>(dur).count();
			if(skip >= MAX_SKIPFRAME || dur > microseconds(DRAW_THRESHOLD_USEC)) {
				skip = 0;
				drawHandler.postArgs(msg::DrawReq(++getInfo()->accumDraw));
			} else
				++skip;
		} while(bLoop && !isInterrupted());

		// 描画スレッドの終了を待つ
		dth.interrupt();
		dth.join();
		guiHandler.postArgs(msg::QuitReq());
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
	GameLoop::GameLoop(MPCreate mcr): _mcr(mcr), _level(Active) {}
	int GameLoop::run(spn::To8Str title, int w, int h, uint32_t flag, int major, int minor, int depth) {
		SDLInitializer	sdlI(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
		Window::SetStdGLAttributes(major, minor, depth);
		SPWindow _spWindow = Window::Create(title.moveTo(), w, h, flag);
		rs::SDLMouse::SetWindow(_spWindow->getWindow());

		// メインスレッドのメッセージキューを初期化
		Looper::Prepare();
		auto& loop = Looper::GetLooper();
		// メインスレッドに渡す
		MainThread mth(_mcr);
		mth.start(std::ref(loop), std::ref(_spWindow));
		// メインスレッドのキューが準備出来るのを待つ
		while(auto msg = loop->wait()) {
			if(msg::MainInit* p = *msg)
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
					if(msg::QuitReq* p = *m) {
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
		mth.getResult();
		return 0;
	}
}

