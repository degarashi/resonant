#pragma once
#include "spinner/singleton.hpp"
#include "tokenmemory.hpp"
#include "sdlwrap.hpp"

namespace rs {
	namespace draw {
		#define mgr_drawtask (::rs::draw::Task::_ref())
		/*! PreFuncとして(TPStructR::applySettingsを追加)
			[Program, FrameBuff, RenderBuff] */
		class Task : public spn::Singleton<Task> {
			public:
				constexpr static int NUM_TASK = 3;
			private:
				//! 描画エントリのリングバッファ
				draw::TokenML	_entry[NUM_TASK];
				HLFxF			_hlFx[NUM_TASK];
				//! 読み書きカーソル位置
				int			_curWrite, _curRead;
				Mutex		_mutex;
				CondV		_cond;

			public:
				Task();
				draw::TokenML& refWriteEnt();
				draw::TokenML& refReadEnt();
				// -------------- from MainThread --------------
				void beginTask(HFx hFx);
				void endTask();
				void clear();
				// -------------- from DrawThread --------------
				void execTask();
		};
	}
}
