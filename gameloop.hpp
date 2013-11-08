#pragma once
#include "event.hpp"
#include "sdlwrap.hpp"
#include "spinner/abstbuff.hpp"

namespace rs {
	template <class T>
	using UPtr = std::unique_ptr<T>;
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
	struct IDrawProc {
		virtual void runU(uint64_t accum) = 0;
	};
	using UPDrawProc = UPtr<IDrawProc>;
	struct IMainProc {
		virtual bool runU() = 0;
		virtual IDrawProc* initDraw() = 0;
	};
	using UPMainProc = UPtr<IMainProc>;
	using MPCreate = std::function<IMainProc* ()>;

	class DrawThread : public ThreadL<void (Looper&, const SPWindow&, const UPMainProc&)> {
		using base = ThreadL<void (Looper&, const SPWindow&)>;
		int 			_state = 0;
		uint64_t		_accum = 0;
		mutable Mutex	_mutex;
		protected:
			void runL(Looper& mainLooper, const SPWindow& w, const UPMainProc& mp) override;
			void setState(int s);
		public:
			int getState() const;
	};
	using UPDrawTh = UPtr<DrawThread>;

	class MainThread : public ThreadL<void (Looper&,const SPWindow&)> {
		using base = ThreadL<void (Looper&, const SPWindow&)>;
		MPCreate	_mcr;
		protected:
			void runL(Looper& guiLooper, const SPWindow& w) override;
		public:
			MainThread(MPCreate mcr);
	};
	using UPMainTh = UPtr<MainThread>;

	const uint32_t EVID_SIGNAL = SDL_RegisterEvents(1);
	int GameLoop(MPCreate mcr, spn::To8Str title, int w, int h, uint32_t flag, int major=2, int minor=0, int depth=16);
}
