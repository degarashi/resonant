#pragma once
#include "updater.hpp"
#include "luaimport.hpp"

namespace rs {
	struct SceneBase;
	#define mgr_scene (::rs::SceneMgr::_ref())
	class SceneMgr : public spn::Singleton<SceneMgr> {
		using StStack = std::vector<HLGbj>;
		StStack		_scene;
		bool		_scOp = false;		//!< SceneOpが有効か
		int			_scNPop;
		HLGbj		_scNext;
		LCValue		_scArg;

		void _doSceneOp();

		public:
			bool isEmpty() const;
			SceneBase& getSceneBase(int n=0) const;
			//! getScene(0)と同義
			HGbj getTop() const;
			HGbj getScene(int n=0) const;
			void setPushScene(HGbj hSc, bool bPop=false);
			void setPopScene(int nPop, const LCValue& arg=LCValue());
			//! フレーム更新のタイミングで呼ぶ
			bool onUpdate();
			//! 描画のタイミングで呼ぶ
			void onDraw();
			void onPause();
			void onStop();
			void onResume();
			void onReStart();
	};

	constexpr static uint32_t SCENE_INTERFACE_ID = 0xf0000000;
	struct SceneBase {
		/*	Updateの優先度はPriority(数値)で表現する
			Drawはグループ分けした上でUpdateと同じようにソート */
		UpdGroup	 update,
					 draw;
		using GroupM = SHandleMap<std::string, HUpd>;
		GroupM		update_m,
					draw_m;

		SceneBase(): update(0), draw(0) {}
	};
	//! 1シーンにつきUpdateTreeとDrawTreeを1つずつ用意
	template <class T>
	class Scene : public ObjectT<T> {
		private:
			using base = ObjectT<T>;
			SceneBase	_sbase;
		public:
			Scene(Priority prio=0): ObjectT<T>(prio) {}
			void onUpdate() override final {
				base::onUpdate();
				if(!base::isDead())
					_sbase.update.onUpdate();
			}
			void onDraw() override final {
				base::onDraw();
				_sbase.draw.onUpdate();
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

