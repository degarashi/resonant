#pragma once
#include "event.hpp"
#include "sdlwrap.hpp"
#include "spinner/abstbuff.hpp"

namespace rs {
	namespace msg {
		struct DrawInit : MsgBase {};
		struct MainInit : MsgBase {};
		//! 描画リクエスト
		struct DrawReq : MsgBase {};
		//! スレッド終了リクエスト
		struct QuitReq : MsgBase {};
		//! スレッド状態値更新
		struct State : MsgBase {
			int state;
			State(int st): state(st) {}
		};
	}
	class DrawThread : public ThreadL<void (Looper&, const SPWindow&)> {
		using base = ThreadL<void (Looper&, const SPWindow&)>;
		int 			_state = 0;
		mutable Mutex	_mutex;
		protected:
			void runL(Looper& drawLooper, const SPWindow& w) override;
			void setState(int s);
		public:
			int getState() const;
	};
	class MainThread : public ThreadL<void (Looper&,const SPWindow&)> {
		using base = ThreadL<void (Looper&, const SPWindow&)>;
		DrawThread	_dth;
		protected:
			void runL(Looper& guiLooper, const SPWindow& w) override;
		public:
	};
	const uint32_t EVID_SIGNAL = SDL_RegisterEvents(1);
	int GameLoop(spn::To8Str title, int w, int h, uint32_t flag, int major=2, int minor=0, int depth=16);
}
