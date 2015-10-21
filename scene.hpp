#pragma once
#include "updater.hpp"
#include "luaimport.hpp"

namespace rs {
	class SceneBase;
	#define mgr_scene (::rs::SceneMgr::_ref())
	//! シーンスタックを管理
	class SceneMgr : public spn::Singleton<SceneMgr> {
		using StStack = std::vector<HLScene>;
		StStack		_scene;
		//! シーンを切り替えや差し替えオペレーションがあるかのフラグ
		bool		_scOp = false;
		int			_scNPop;
		HLScene		_scNext;
		LCValue		_scArg;

		void _doSceneOp();

		public:
			bool isEmpty() const;
			//! シーンスタック中のSceneBaseを取得
			IScene& getSceneInterface(int n=0) const;
			//! ヘルパー関数: シーンスタック中のUpdGroupを取得
			/*! *getSceneBase(n).update->get() と同等 */
			UpdGroup& getUpdGroupRef(int n=0) const;
			//! ヘルパー関数: シーンスタック中のDrawGroupを取得
			/*! *getSceneBase(n).draw->get() と同等 */
			DrawGroup& getDrawGroupRef(int n=0) const;
			//! getScene(0)と同義
			HScene getTop() const;
			HScene getScene(int n=0) const;
			void setPushScene(HScene hSc, bool bPop=false);
			void setPopScene(int nPop, const LCValue& arg=LCValue());
			//! フレーム更新のタイミングで呼ぶ
			bool onUpdate();
			//! 描画のタイミングで呼ぶ
			void onDraw(IEffect& e);
			void onPause();
			void onStop();
			void onResume();
			void onReStart();
	};

	class SceneBase {
		private:
			DefineUpdGroup(Update)
			DefineDrawGroup(Draw)

			HLGroup		_update;
			HLDGroup	_draw;

		public:
			SceneBase(HGroup hUpd, HDGroup hDraw);
			void setUpdate(HGroup hGroup);
			HGroup getUpdate() const;
			void setDraw(HDGroup hDGroup);
			HDGroup getDraw() const;
	};
	//! 1シーンにつきUpdateTreeとDrawTreeを1つずつ用意
	template <class T>
	class Scene : public ObjectT_Lua<T, IScene> {
		private:
			using base = ObjectT_Lua<T, IScene>;
			SceneBase	_sbase;
		public:
			Scene(HGroup hUpd=HGroup(), HDGroup hDraw=HDGroup()):
				_sbase(hUpd, hDraw) {}
			void onUpdate(bool /*bFirst*/) override final {
				base::onUpdate(true);
				if(!base::isDead()) {
					UpdGroup::SetAsUpdateRoot();
					_sbase.getUpdate()->get()->onUpdate(true);
				}
			}
			void onDraw(IEffect& e) const override final {
				base::onDraw(e);
				_sbase.getDraw()->get()->onDraw(e);
			}
			void onDisconnected(HGroup h) override final {
				AssertP(Trap, !h)
				_sbase.getUpdate()->get()->onDisconnected(h);
				base::onDisconnected(h);
			}
			void onConnected(HGroup h) override final {
				base::onConnected(h);
				AssertP(Trap, !h)
				_sbase.getUpdate()->get()->onConnected(h);
			}
			//! ヘルパー関数: シーンスタック中のUpdGroupを取得
			/*! *getBase().update->get() と同等 */
			UpdGroup& getUpdGroupRef() const {
				return *getBase().getUpdate()->get();
			}
			HGroup getUpdGroup() const override {
				return getBase().getUpdate();
			}
			//! ヘルパー関数: シーンスタック中のDrawGroupを取得
			/*! *getBase().draw->get() と同等 */
			DrawGroup& getDrawGroupRef() const {
				return *getBase().getDraw()->get();
			}
			HDGroup getDrawGroup() const override {
				return getBase().getDraw();
			}
			const SceneBase& getBase() const {
				return _sbase;
			}
			SceneBase& getBase() {
				return _sbase;
			}
			#define DEF_ADAPTOR(name) void name() override final { \
				base::getState()->name(base::getRef()); \
				base::_doSwitchState(); }
			DEF_ADAPTOR(onPause)
			DEF_ADAPTOR(onStop)
			DEF_ADAPTOR(onResume)
			DEF_ADAPTOR(onReStart)
			#undef DEF_ADAPTOR
	};
	class U_Scene : public Scene<U_Scene> {
		private:
			struct St_None;
		public:
			U_Scene();
	};
}
DEF_LUAIMPORT(rs::SceneMgr)
DEF_LUAIMPORT(rs::U_Scene)

