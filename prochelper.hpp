#pragma once
#include "spinner/size.hpp"
#include "updater.hpp"
#include "scene.hpp"
#include "gameloop.hpp"
#include "glhead.hpp"

namespace rs {
	class Window;
	using SPWindow = std::shared_ptr<Window>;
	struct SharedBaseValue {
		HLFx		hlFx;
		HInput		hlIk,
					hlIm;
		FPSCounter	fps;
		spn::Size	screenSize;
		SPWindow	spWindow;
	};
	constexpr int Id_SharedBaseValueC = 0xf0000000;
	#define sharedbase	(::rs::SharedBaseValueC::_ref())
	class SharedBaseValueC : public spn::Singleton<SharedBaseValueC>, public GLSharedData<SharedBaseValue, Id_SharedBaseValueC> {};

	class DrawProc : public IDrawProc {
		public:
			bool runU(uint64_t accum, bool bSkip) override;
	};
	class MainProc : public IMainProc {
		private:
			SharedBaseValueC	_sbvalue;
			bool				_bFirstScene;
			void	_initInput();
			bool	_beginProc();
			void	_endProc();
		protected:
			void	_pushFirstScene(HGbj hGbj);
			virtual bool userRunU() = 0;
		public:
			MainProc(const SPWindow& sp, bool bInitInput);
			bool runU(IMainProc::Query& q) override final;
			void onPause() override;
			void onResume() override;
			void onStop() override;
			void onReStart() override;
	};
}
