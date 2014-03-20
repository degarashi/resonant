#pragma once
#include "updator.hpp"
#include "luaimport.hpp"

namespace rs {
	#define mgr_scene (::rs::SceneMgr::_ref())
	class SceneMgr : public spn::Singleton<SceneMgr> {
		using StStack = std::vector<HLGbj>;
		StStack		_scene;
		bool		_scOp = false;		//!< SceneOpが有効か
		int			_scNPop;
		HLGbj		_scNext;
		Variant		_scArg;

		void _doSceneOp();

		public:
			bool isEmpty() const;
			//! getScene(0)と同義
			HGbj getTop() const;
			HGbj getScene(int n) const;
			void setPushScene(HGbj hSc, bool bPop=false);
			void setPopScene(int nPop, const Variant& arg=Variant());
			//! フレーム更新のタイミングで呼ぶ
			bool onUpdate();
			//! 描画のタイミングで呼ぶ
			void onDraw();
			void onPause();
			void onStop();
			void onResume();
			void onReStart();
	};

	//! 1シーンにつきUpdateTreeとDrawTreeを1つずつ用意
	template <class T>
	class Scene : public ObjectT<T> {
		using base = ObjectT<T>;
		protected:
			UpdGroup _update, _draw;
		public:
			Scene(Priority prio=0): ObjectT<T>(prio), _update(0), _draw(0) {}
			void onUpdate() override final {
				base::onUpdate();
				if(!ObjectT<T>::isDead())
					_update.onUpdate();
			}
			void onDraw() override final {
				base::onDraw();
				_draw.onUpdate();
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

