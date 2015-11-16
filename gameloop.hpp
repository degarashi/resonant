#pragma once
#include "serialization/chars.hpp"
#include "spinner/abstbuff.hpp"
#include "event.hpp"
#include "sdlwrap.hpp"
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
		//! OpenGLコンテキスト生成リクエスト(Main -> Draw)
		struct MakeContext : MsgBase<MakeContext> {};
		//! OpenGLコンテキスト破棄リクエスト(Main -> Draw)
		struct DestroyContext : MsgBase<DestroyContext> {};

		//! 描画リクエスト
		struct DrawReq : MsgBase<DrawReq> {
			// 管理用の描画リクエストID
			uint64_t	id;
			bool		bSkip;
			DrawReq(uint64_t t, bool skip): id(t), bSkip(skip) {}
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
			\param[in] bSkip 描画スキップフラグ
			\return backbufferのswapをかける時はtrue */
		virtual bool runU(uint64_t accum, bool bSkip) = 0;
		virtual ~IDrawProc() {}
	};
	namespace detail {
		extern const bool c_pauseDefault;
	}
	using UPDrawProc = UPtr<IDrawProc>;
	struct IMainProc {
		class Query {
			private:
				bool		_bDraw;
				Timepoint	_tp;
				int			_skip;
			public:
				Query(Timepoint tp, int skip);
				bool canDraw() const;
				void setDraw(bool bDraw);
				bool getDraw() const;
		};
		virtual bool runU(Query& q) = 0;		//!< 毎フレームのアップデート処理
		virtual bool onPause() { return detail::c_pauseDefault; }
		virtual void onResume() {}
		virtual void onStop() {}
		virtual void onReStart() {}

		virtual ~IMainProc() {}
	};
	using UPMainProc = UPtr<IMainProc>;
	using MPCreate = std::function<IMainProc* (const SPWindow&)>;
	using DPCreate = std::function<IDrawProc* ()>;

	//! FPS計測クラス
	/*! GameLoop内で使用 */
	struct FPSCounter {
		Timepoint	_tmBegin;
		int			_counter,
					_fps;

		FPSCounter();
		void reset();
		bool update();
		int getFPS() const;
	};
	#define draw_thread (::rs::DrawThread::_ref())
	//! 描画スレッド
	class DrawThread : public spn::Singleton<DrawThread>,
						public ThreadL<void (const SPLooper&, const SPWindow&, IDrawProc*)>
	{
		public:
			enum class State {
				Idle,
				Drawing
			};
		private:
			using base = ThreadL<void (Looper&, const SPWindow&, IDrawProc*)>;
			struct Info {
				State		state = State::Idle;	//!< 現在の動作状態(描画中か否か)
				uint64_t	accum = 0;				//!< 描画が終わったフレーム番号
				// SingleContext環境ではctxMainThread = nullptr
				SPGLContext	ctxDrawThread,			//!< 描画スレッド用のGLコンテキスト
							ctxMainThread;			//!< メインスレッド用のGLコンテキスト
				FPSCounter	fps;
			};
			SpinLock<Info>		_info;
		protected:
			void runL(const SPLooper& mainLooper, const SPWindow& w, IDrawProc*) override;
		public:
			DrawThread();
			auto getInfo() -> decltype(_info.lock()) { return _info.lock(); }
			auto getInfo() const -> decltype(_info.lockC()) { return _info.lockC(); }
	};
	#define main_thread (::rs::MainThread::_ref())

	struct GLoopInitializer {
		using CB = std::function<void ()>;
		CB	cbInit,			//!< 各種マネージャが初期化された後に呼ばれる
			cbEngineInit,	//!< 描画エンジンが初期化された後に呼ばれる
			cbPreTerm,		//!< 各種マネージャが破棄される前に呼ばれる
			cbPostTerm;		//!< 各種マネージャが破棄された後に呼ばれる
	};
	struct GLoopInitParam {
		std::string		pathfile,		//!< パス記述ファイル名
						organization,	//!< 組織名(一時ファイル用)
						app_name;		//!< アプリケション名(一時ファイル用)
		GLoopInitializer initializer;
		Window::Param	wparam;
		Window::GLParam	gparam;
	};
	//! メインスレッド
	class MainThread : public spn::Singleton<MainThread>,
						public ThreadL<void (const SPLooper&,const SPWindow&,const GLoopInitParam&)>
	{
		using base = ThreadL<void (const SPLooper&,const SPWindow&,const char*)>;
		MPCreate	_mcr;
		DPCreate	_dcr;

		struct Info {
			uint64_t	accumUpd;	//!< アップデート累積カウンタ
			uint64_t	accumDraw;	//!< 描画フレーム累積カウンタ
			Timepoint	tmBegin;	//!< ゲーム開始時の時刻
		};
		SpinLock<Info>		_info;

		protected:
			void runL(const SPLooper& guiLooper, const SPWindow& w, const GLoopInitParam& param) override;
		public:
			MainThread(MPCreate mcr, DPCreate dcr);
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
		void _procMouseEvent(SDL_Event& e);

		using OPHandler = spn::Optional<Handler>;
		MPCreate	_mcr;
		DPCreate	_dcr;
		OPHandler	_handler;
		Level		_level;

		public:
			/*!	\param[in] mcr ゲームアップデートコールバック(メインスレッドから呼ばれる, ゲーム終了時にfalseを返す)
				\param[in] dcr 描画コールバックインタフェースを作成(描画スレッドから呼ばれる) */
			GameLoop(MPCreate mcr, DPCreate dcr);

			/*! \retval 0		正常に終了
				\retval 0以外	異常終了 */
			int run(const GLoopInitParam& param);
			//! AppPathの場所に置かれたをフォントファイルを列挙し、読み込む
			static void LoadFonts();
			//! AppPathをテキストファイルから設定
			static void LoadPathfile(const spn::URI& uri, bool bAppend=false);
	};
}

