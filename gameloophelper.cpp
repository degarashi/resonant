#include "gameloophelper.hpp"
#include "adaptsdl.hpp"
#include "input.hpp"
#include "glresource.hpp"
#include "glx_if.hpp"

namespace rs {
	namespace detail {
		// ----------------- GameloopHelper -----------------
		int GameloopHelper::Run(const CBEngine& cbEngine, const CBMakeSV& cbMakeSV, const CBInit& cbInit, const CBScene& cbScene, const CBInit& cbTerm, int rx, int ry, const std::string& appname, const std::string& pathfile) {
			GameLoop gloop([&](const SPWindow& sp){
					return new GHelper_Main(sp, cbEngine, cbMakeSV, cbInit, cbTerm, cbScene);
					}, [](){ return new GHelper_Draw; });

			GLoopInitParam param;
			auto& wp = param.wparam;
			wp.title = appname + " by RSE";
			wp.width = rx;
			wp.height = ry;
			wp.flag = SDL_WINDOW_SHOWN;
			auto& gp = param.gparam;
			gp.depth = 24;
			gp.verMajor = 2;
			gp.verMinor = 0;
			param.pathfile = pathfile;
			param.app_name = appname;
			param.organization = "default_org";
			return gloop.run(param);
		}

		// ----------------- GHelper_Draw -----------------
		bool GHelper_Draw::runU(uint64_t accum, bool bSkip) {
			return DrawProc::runU(accum, bSkip);
		}

		// ----------------- GHelper_Main -----------------
		GHelper_Main::GHelper_Main(const SPWindow& sp, const CBEngine& cbEngine, const CBMakeSV& cbMakeSV, const CBInit& cbInit, const CBInit& cbTerm, const CBScene& cbScene):
			MainProc(sp, true),
			_cbTerm(cbTerm)
		{
			auto lkb = sharedbase.lock();

			// GLEffectは名前固定: default.glx
			lkb->hlFx = mgr_gl.loadEffect("default.glx", cbEngine);
			_upsv = cbMakeSV();
			// Luaスクリプトの初期化
			// スクリプトファイルの名前は固定: main.lua
			SPLua ls = lkb->spLua = LuaState::FromResource("main");
			LuaImport::RegisterRSClass(*ls);
			cbInit();
			// Lua初期化関数を呼ぶ
			ls->getGlobal("Initialize");
			ls->call(0,1);

			HLScene hlScene;
			if(ls->type(-1) != LuaType::Nil)
				hlScene = LCV<HScene>()(-1, ls->getLS());
			else
				hlScene = cbScene();
			_pushFirstScene(hlScene);
		}
		GHelper_Main::~GHelper_Main() {
			_cbTerm();
		}
		bool GHelper_Main::userRunU() {
			return true;
		}
	}
}

