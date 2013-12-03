#include "gameloop.hpp"
#include "input.hpp"
#include "font.hpp"
#include "spinner/frac.hpp"
#include "camera.hpp"

namespace rs {
	// --------------------- DrawThread ---------------------
	void DrawThread::runL(const SPLooper& mainLooper, SPGLContext&& ctx_b, const SPWindow& w, const UPMainProc& mp) {
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
					setState(1);
					// 1フレーム分の描画処理
					if(up->runU(_accum++))
						ctx->swapWindow();
					setState(0);
				}
			}
		} while(bLoop && !isInterrupted());
	}
	void DrawThread::setState(int s) {
		UniLock lk(_mutex);
		_state = s;
	}
	int DrawThread::getState() const {
		UniLock lk(_mutex);
		return _state;
	}

	// --------------------- MainThread ---------------------
	MainThread::MainThread(MPCreate mcr): _mcr(mcr) {}
	void MainThread::runL(const SPLooper& guiLooper, const SPWindow& w) {
		GLRes 		glrP;
		RWMgr 		rwP;
		FontFamily	fontP;
		fontP.loadFamilyWildCard("/home/slice/.fonts/*.ttc");
		FontGen		fgenP(spn::PowSize(512,512));
		CameraMgr	camP;
		InputMgr	inpP;

		UPMainProc mp(_mcr(w));
		Handler guiHandler(guiLooper);
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
				guiHandler.postArgs(msg::QuitReq());
				SDL_Event e;
				e.type = EVID_SIGNAL;
				SDLEC_P(Warn, SDL_PushEvent, &e);
				// 何らかの原因で描画スレッドが終了していた時
				try {
					// 例外が投げられて終了したかをチェック
					dth.getResult();
				} catch (...) {
					Assert(Warn, false, "MainThread: draw thread was ended by throwing exception")
					throw;
				}
				Assert(Warn, false, "MainThread: draw thread was ended unexpectedly")
				break;
			}
			// 何かメッセージが来てたら処理する
			while(OPMessage m = getLooper()->peek(std::chrono::seconds(0))) {
			}
			// 次のフレーム開始を待つ
			auto ntp = prevtime + microseconds(16666);
			auto tp = Clock::now();
			if(ntp > tp) {
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
			mgr_input.update();
			if(!mp->runU())
				break;

			// 時間が残っていれば描画
			// 最大スキップフレームを超過してたら必ず描画
			auto dur = Clock::now() - tp;
			if(skip < MAX_SKIPFRAME && dur > microseconds(DRAW_THRESHOLD_USEC)) {
				skip = 0;
				drawHandler.postArgs(msg::DrawReq());
			} else
				++skip;
		} while(bLoop && !isInterrupted());

		// 描画スレッドの終了を待つ
		dth.interrupt();
		dth.join();
	}

	int GameLoop(MPCreate mcr, spn::To8Str title, int w, int h, uint32_t flag, int major, int minor, int depth) {
		SDLInitializer	sdlI(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_JOYSTICK | SDL_INIT_TIMER);
		Window::SetStdGLAttributes(major, minor, depth);
		SPWindow _spWindow = Window::Create(title.moveTo(), w, h, flag);
		rs::SDLMouse::SetWindow(_spWindow->getWindow());

		// メインスレッドのメッセージキューを初期化
		Looper::Prepare();
		auto& loop = Looper::GetLooper();
		// メインスレッドに渡す
		MainThread mth(mcr);
		mth.start(std::ref(loop), std::ref(_spWindow));
		// メインスレッドのキューが準備出来るのを待つ
		while(auto msg = loop->wait()) {
			if(msg::MainInit* p = *msg)
				break;
		}
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
				switch(e.type) {
					case SDL_WINDOWEVENT:
						// ウィンドウ閉じたら終了
						if(e.window.event == SDL_WINDOWEVENT_CLOSE) {
							e.type = SDL_QUIT;
							e.quit.timestamp = 0;
							SDL_PushEvent(&e);
						}
						break;
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
						break;
					case SDL_APP_WILLENTERFOREGROUND:	// Android: onResume()
					case SDL_APP_DIDENTERFOREGROUND:	// Android: onResume()
						break;
				}
			}
		}
		mth.interrupt();
		mth.getResult();
		return 0;
	}
}
