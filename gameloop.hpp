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
		//! 描画コールバック
		/*! 描画スレッドから呼ばれる */
		/*! \param[in] accum 累積フレーム数
			\return backbufferのswapをかける時はtrue */
		virtual bool runU(uint64_t accum) = 0;
		virtual ~IDrawProc() {}
	};
	using UPDrawProc = UPtr<IDrawProc>;
	struct IMainProc {
		//! ゲームアップデートコールバック
		/*! メインスレッドから呼ばれる */
		/*! \return ゲーム終了時にfalseを返す */
		virtual bool runU() = 0;
		//! 描画コールバックインタフェースを作成
		/*! 描画スレッドから呼ばれる */
		virtual IDrawProc* initDraw() = 0;
		virtual ~IMainProc() {}
	};
	using UPMainProc = UPtr<IMainProc>;
	using MPCreate = std::function<IMainProc* (const SPWindow&)>;

	class DrawThread : public ThreadL<void (const SPLooper&, SPGLContext&&, const SPWindow&, const UPMainProc&)> {
		using base = ThreadL<void (Looper&, SPGLContext&&, const SPWindow&, const UPMainProc&)>;
		int 			_state = 0;
		uint64_t		_accum = 0;
		mutable Mutex	_mutex;
		protected:
			void runL(const SPLooper& mainLooper, SPGLContext&& ctx_b, const SPWindow& w, const UPMainProc& mp) override;
			void setState(int s);
		public:
			int getState() const;
	};
	using UPDrawTh = UPtr<DrawThread>;

	class MainThread : public ThreadL<void (const SPLooper&,const SPWindow&)> {
		using base = ThreadL<void (Looper&, const SPWindow&)>;
		MPCreate	_mcr;
		protected:
			void runL(const SPLooper& guiLooper, const SPWindow& w) override;
		public:
			MainThread(MPCreate mcr);
	};
	using UPMainTh = UPtr<MainThread>;

	const uint32_t EVID_SIGNAL = SDL_RegisterEvents(1);
	int GameLoop(MPCreate mcr, spn::To8Str title, int w, int h, uint32_t flag, int major=2, int minor=0, int depth=16);
}
