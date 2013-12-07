#pragma once
#include "event.hpp"
#include "sdlwrap.hpp"
#include "spinner/abstbuff.hpp"
#include "clock.hpp"

namespace rs {
	struct PrintEvent {
		enum Type {
			EVENT_WINDOW = 0x01,
			EVENT_TOUCH = 0x02,
			NUM_EVENT = 2,
			ALL_EVENT = ~0
		};
		using Checker = std::function<bool (uint32_t)>;
		using PrintF = bool (*)(const SDL_Event&);
		struct TypeP {
			Checker	checker;
			PrintF	proc;
		};
		const static TypeP cs_type[NUM_EVENT];

		static bool Window(const SDL_Event& e);
		static bool Touch(const SDL_Event& e);
		static void All(const SDL_Event& e, uint32_t filter = ALL_EVENT);
	};

	template <class T>
	using UPtr = std::unique_ptr<T>;
	namespace msg {
		// ---- 初期化時 ----
		struct DrawInit : MsgBase<DrawInit> {};
		struct MainInit : MsgBase<MainInit> {};
		// ---- ステート遷移 ----
		struct PauseReq : MsgBase<PauseReq> {};
		struct ResumeReq : MsgBase<ResumeReq> {};
		struct StopReq : MsgBase<StopReq> {};
		struct ReStartReq : MsgBase<ReStartReq> {};

		//! 描画リクエスト
		struct DrawReq : MsgBase<DrawReq> {
			// 管理用の描画リクエストID
			uint64_t	id;
			DrawReq(uint64_t t): id(t) {}
		};
		//! スレッド終了リクエスト
		struct QuitReq : MsgBase<QuitReq> {};
		//! スレッド状態値更新
		struct State : MsgBase<State> {
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
		virtual void onPause() {}
		virtual void onResume() {}
		virtual void onStop() {}
		virtual void onReStart() {}

		virtual ~IMainProc() {}
	};
	using UPMainProc = UPtr<IMainProc>;
	using MPCreate = std::function<IMainProc* (const SPWindow&)>;

	#define draw_thread (::rs::DrawThread::_ref())
	//! 描画スレッド
	class DrawThread : public spn::Singleton<DrawThread>,
						public ThreadL<void (const SPLooper&, SPGLContext, const SPWindow&, const UPMainProc&)>
	{
		public:
			enum class State {
				Idle,
				Drawing
			};
		private:
			using base = ThreadL<void (Looper&, SPGLContext, const SPWindow&, const UPMainProc&)>;
			struct Info {
				State		state = State::Idle;	//!< 現在の動作状態(描画中か否か)
				uint64_t	accum = 0;				//!< 描画が終わったフレーム番号
			};
			SpinLock<Info>		_info;
		protected:
			void runL(const SPLooper& mainLooper, SPGLContext ctx_b, const SPWindow& w, const UPMainProc& mp) override;
		public:
			auto getInfo() -> decltype(_info.lock()) { return _info.lock(); }
			auto getInfo() const -> decltype(_info.lockC()) { return _info.lockC(); }
	};
	#define main_thread (::rs::MainThread::_ref())
	//! メインスレッド
	class MainThread : public spn::Singleton<MainThread>,
						public ThreadL<void (const SPLooper&,const SPWindow&)>
	{
		using base = ThreadL<void (const SPLooper&,const SPWindow&)>;
		MPCreate	_mcr;

		struct Info {
			uint64_t	accumUpd;	//!< アップデート累積カウンタ
			uint64_t	accumDraw;	//!< 描画フレーム累積カウンタ
			Timepoint	tmBegin;	//!< ゲーム開始時の時刻
			int			fps;		//!< FPS値
		};
		SpinLock<Info>		_info;

		protected:
			void runL(const SPLooper& guiLooper, const SPWindow& w) override;
		public:
			MainThread(MPCreate mcr);
			auto getInfo() -> decltype(_info.lock()) { return _info.lock(); }
			auto getInfo() const -> decltype(_info.lockC()) { return _info.lockC(); }
	};

	extern const uint32_t EVID_SIGNAL;
	#define gui_thread (::rs::GameLoop::_ref())
	//! GUIスレッド
	class GameLoop : public spn::Singleton<GameLoop> {
		void _onPause();
		void _onResume();
		void _onStop();
		void _onReStart();

		enum Level {
			/*! ゲーム停止。リソース解放
				Android: OnPauseの時
				Desktop: 最小化された時 */
			Stop,
			/*! ゲーム一時停止。リソースは開放しない
				Desktop: フォーカスが外れた時 */
			Pause,
			//! ゲーム進行中
			Active,
			NumLevel
		};
		using LFunc = void (GameLoop::*)();
		const static LFunc cs_lfunc[NumLevel][2];
		void _setLevel(Level level);
		void _procWindowEvent(SDL_Event& e);

		using OPHandler = spn::Optional<Handler>;
		MPCreate	_mcr;
		OPHandler	_handler;
		Level		_level;

		public:
			GameLoop(MPCreate mcr);
			int run(spn::To8Str title, int w, int h, uint32_t flag, int major=2, int minor=0, int depth=16);
	};
}
