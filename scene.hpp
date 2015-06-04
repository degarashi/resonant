#pragma once
#include "updater.hpp"
#include "luaimport.hpp"

namespace rs {
	class SceneBase;
	#define mgr_scene (::rs::SceneMgr::_ref())
	//! シーンスタックを管理
	class SceneMgr : public spn::Singleton<SceneMgr> {
		using StStack = std::vector<HLObj>;
		StStack		_scene;
		//! シーンを切り替えや差し替えオペレーションがあるかのフラグ
		bool		_scOp = false;
		int			_scNPop;
		HLObj		_scNext;
		LCValue		_scArg;

		void _doSceneOp();

		public:
			bool isEmpty() const;
			//! シーンスタック中のSceneBaseを取得
			SceneBase& getSceneBase(int n=0) const;
			//! ヘルパー関数: シーンスタック中のUpdGroupを取得
			/*! *getSceneBase(n).update->get() と同等 */
			UpdGroup& getUpdGroup(int n=0) const;
			//! ヘルパー関数: シーンスタック中のDrawGroupを取得
			/*! *getSceneBase(n).draw->get() と同等 */
			DrawGroup& getDrawGroup(int n=0) const;
			//! getScene(0)と同義
			HObj getTop() const;
			HObj getScene(int n=0) const;
			void setPushScene(HObj hSc, bool bPop=false);
			void setPopScene(int nPop, const LCValue& arg=LCValue());
			//! フレーム更新のタイミングで呼ぶ
			bool onUpdate();
			//! 描画のタイミングで呼ぶ
			void onDraw(GLEffect& e);
			void onPause();
			void onStop();
			void onResume();
			void onReStart();
	};

	constexpr static uint32_t SCENE_INTERFACE_ID = 0xf0000000;
	class SceneBase {
		private:
			DefineGroupT(Update, UpdGroup)
			DefineGroupT(Draw, DrawGroup)

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
	class Scene : public ObjectT<T> {
		private:
			using base = ObjectT<T>;
			SceneBase	_sbase;
		public:
			Scene(HGroup hUpd=HGroup(), HDGroup hDraw=HDGroup()):
				_sbase(hUpd, hDraw) {}
			void onUpdate() override final {
				UpdGroup::SetAsUpdateRoot();
				base::onUpdate();
				if(!base::isDead())
					_sbase.getUpdate()->get()->onUpdate();
			}
			void onDraw(GLEffect& e) const override final {
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
			UpdGroup& getUpdGroup() const {
				return *getBase().getUpdate()->get();
			}
			//! ヘルパー関数: シーンスタック中のDrawGroupを取得
			/*! *getBase().draw->get() と同等 */
			DrawGroup& getDrawGroup() const {
				return *getBase().getDraw()->get();
			}
			const SceneBase& getBase() const {
				return _sbase;
			}
			SceneBase& getBase() {
				return _sbase;
			}
			void* getInterface(uint32_t id) override {
				if(id == SCENE_INTERFACE_ID)
					return &_sbase;
				return base::getInterface(id);
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
	class U_Scene : public Scene<U_Scene> {};
}
DEF_LUAIMPORT(SceneMgr)
DEF_LUAIMPORT(U_Scene)

