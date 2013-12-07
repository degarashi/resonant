#pragma once
#include "updator.hpp"

namespace rs {
	#define mgr_scene (::rs::SceneMgr::_ref())
	class SceneMgr : public spn::Singleton<SceneMgr> {
		using StStack = std::stack<HLGbj>;
		StStack		_scene;
		bool		_scOp = false;		//!< SceneOpが有効か
		int			_scNPop;
		HLGbj		_scNext;
		Variant		_scArg;

		void _doSceneOp();

		public:
			bool isEmpty() const;
			HGbj getTop() const;
			void setPushScene(HGbj hSc, bool bPop=false);
			void setPopScene(int nPop, const Variant& arg=Variant());
			//! フレーム更新のタイミングで呼ぶ
			bool onUpdate();
			//! 描画のタイミングで呼ぶ
			void onDraw();
	};

	//! 1シーンにつきUpdateTreeとDrawTreeを1つずつ用意
	template <class T>
	class Scene : public ObjectT<T> {
		protected:
			UpdGroup _update, _draw;
		public:
			Scene(Priority prio=0): ObjectT<T>(prio), _update(0), _draw(0) {}
			void onUpdate() override final {
				ObjectT<T>::onUpdate();
				if(!ObjectT<T>::isDead())
					_update.onUpdate();
			}
			void onDraw() override final {
				ObjectT<T>::onDraw();
				_draw.onUpdate();
			}
	};
}
