#pragma once
#include "handle.hpp"
#include "prochelper.hpp"
#include <unordered_set>

namespace rs {
	struct AdaptSDL;
	namespace detail {
		struct ISharedBase {
			virtual ~ISharedBase() {}
		};
		using SharedBase_UP = std::unique_ptr<ISharedBase>;
		template <class T>
		struct SharedBase : ISharedBase {
			std::unique_ptr<T>	_ptr;
			SharedBase(): _ptr(new T) {}
		};

		using CBMakeSV = std::function<SharedBase_UP ()>;
		using CBEngine = std::function<IEffect* (const std::string&)>;
		using CBScene = std::function<HLScene ()>;
		//! ゲームループヘルパークラスの実装
		class GameloopHelper {
			public:
				/*! \param[in] cbEngine		Engineクラスの初期化関数
					\param[in] cbMakeSV		ゲーム内グローバル変数の初期化関数
					\param[in] init			ゲームループ初期化関数群
					\param[in] cbScene		最初のシーンクラスを生成して返す関数 */
				static int Run(const CBEngine& cbEngine,
								const CBMakeSV& cbMakeSV,
								const GLoopInitializer& init,
								const CBScene& cbScene,
								int rx, int ry, const std::string& appname, const std::string& pathfile);
		};
		//! 描画スレッド内の処理代行
		class GHelper_Draw : public DrawProc {
			public:
				bool runU(uint64_t accum, bool bSkip) override;
		};
		//! ゲームスレッド内の処理代行
		class GHelper_Main : public MainProc {
			private:
				//! ゲーム内グローバル変数
				detail::SharedBase_UP	_upsv;
				CBEngine				_cbEngine;
				GLoopInitializer		_init;
				using PathSet = std::unordered_set<std::string>;
				PathSet					_updatePath;
				UPResource				_prevFx;
				int						_prevFxCounter;
			public:
				GHelper_Main(const SPWindow& sp,
							const CBEngine& cbEngine,
							const CBMakeSV& cbMakeSV,
							const GLoopInitializer& init,
							const CBScene& cbScene);
				~GHelper_Main();
				bool userRunU() override;
		};
	}

	//! ゲームループヘルパークラス (インタフェース)
	/*! 従来GameLoopクラスを使うのに
		UserThreadProc, DrawThreadProc, Engineの3つのクラス定義が必要だった所を
		Scene, SharedData, Engineを定義するだけで済むようにした物 */
	template <class EngineT, class SharedT, class InitialSceneT>
	class GameloopHelper : public detail::GameloopHelper {
		public:
			constexpr static int Id_SharedValueC = 0xf0000001;
			class SharedValueC :
				public spn::Singleton<SharedValueC>,
				public GLSharedData<SharedT, Id_SharedValueC> {};

			//! ゲームループの初期化及び進行
			/*! \param[in] cbInit	ゲームループの初期化が完了したタイミングで呼ばれる
				\param[in] rx		画面解像度(縦)
				\param[in] ry		画面解像度(横)
				\param[in] appname	アプリケーション名(一時ファイルパスやウィンドウタイトルに使用)
				\param[in] pathfile	各リソースへのパスを記述したテキストファイルパス
				\retval 0		正常終了
				\retval 0以外	異常終了
			*/
			static int Run(const GLoopInitializer& init, int rx, int ry, const std::string& appname, const std::string& pathfile="") {
				auto cbEngine = [](auto& name){
					return new EngineT(name);
				};
				auto cbMakeSV = [](){
					return detail::SharedBase_UP(
								new detail::SharedBase<SharedValueC>()
							);
				};
				auto cbScene = [](){
					return rs_mgr_obj.makeScene<InitialSceneT>().first;
				};
				return detail::GameloopHelper::Run(cbEngine, cbMakeSV, init, cbScene, rx, ry, appname, pathfile);
			}
	};
}
