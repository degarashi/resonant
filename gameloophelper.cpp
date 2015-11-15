#include "gameloophelper.hpp"
#include "adaptsdl.hpp"
#include "input.hpp"
#include "glresource.hpp"
#include "glx_if.hpp"
#include "spinner/watch.hpp"

namespace rs {
	namespace detail {
		// ----------------- GameloopHelper -----------------
		int GameloopHelper::Run(const CBEngine& cbEngine, const CBMakeSV& cbMakeSV, const GLoopInitializer& init, const CBScene& cbScene, int rx, int ry, const std::string& appname, const std::string& pathfile) {
			GameLoop gloop([&](const SPWindow& sp){
					return new GHelper_Main(sp, cbEngine, cbMakeSV, init, cbScene);
					}, [](){ return new GHelper_Draw; });

			GLoopInitParam param;
			param.initializer = init;
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

		namespace {
			const std::string c_fxfile = "default.glx";
		}
		// ----------------- GHelper_Main -----------------
		GHelper_Main::GHelper_Main(const SPWindow& sp, const CBEngine& cbEngine, const CBMakeSV& cbMakeSV, const GLoopInitializer& init, const CBScene& cbScene):
			MainProc(sp, true),
			_cbEngine(cbEngine),
			_init(init)
		{
			auto lkb = sharedbase.lock();

			// GLEffectは名前固定: default.glx
			lkb->hlFx = mgr_gl.loadEffect(c_fxfile, cbEngine);
			_upsv = cbMakeSV();
			// Luaスクリプトの初期化
			// スクリプトファイルの名前は固定: main.lua
			SPLua ls = lkb->spLua = LuaState::FromResource("main");
			LuaImport::RegisterRSClass(*ls);
			init.cbInit();
			if(_init.cbEngineInit)
				_init.cbEngineInit();
			rs_mgr_obj.setLua(lkb->spLua);
			// Lua初期化関数を呼ぶ
			ls->getGlobal("Initialize");
			ls->call(0,1);

			// シェーダーファイルが置いてあるディレクトリを監視
			auto& ntf = lkb->spNtf;
			ntf = std::make_shared<spn::FNotify>();
			mgr_path.enumPath("effect", "*.*", [&ntf](const spn::Dir& d) {
				if(d.getLast_utf8() == c_fxfile) {
					ntf->addWatch(d.getSegment_utf8(0, d.segments()-1), spn::FE_Modify|spn::FE_Create|spn::FE_MoveTo);
					return false;
				}
				return true;
			});

			HLScene hlScene;
			if(ls->type(-1) != LuaType::Nil)
				hlScene = LCV<HScene>()(-1, ls->getLS());
			else
				hlScene = cbScene();
			_pushFirstScene(hlScene);
		}
		GHelper_Main::~GHelper_Main() {
			if(_init.cbPreTerm)
				_init.cbPreTerm();
		}
		bool GHelper_Main::userRunU() {
			// シェーダーファイルが更新されていたら再読み込みをかける
			auto lkb = sharedbase.lock();
			bool bUpdate = false;
			lkb->spNtf->procEvent([&bUpdate, &path=_updatePath](const spn::FEv& e, const spn::SPData&){
				path.emplace(*e.basePath + '/' + e.name);
				bUpdate = true;
			});
			// 連続したファイル操作の直後に再読み込み
			if(!bUpdate && !_updatePath.empty()) {
				try {
					// ブロック(GLXStruct)をすべてリロードしてから
					bool bLoad = false;
					for(auto& p : _updatePath) {
						spn::Dir dir(p);
						if(dir.isFile()) {
							ReloadFxBlock(spn::URI("file", dir.plain_utf32()));
							bLoad = true;
						}
					}
					_updatePath.clear();
					if(bLoad) {
						// Effectファイルの再構築
						mgr_gl.replaceEffect(lkb->hlFx, _cbEngine);
						if(_init.cbEngineInit)
							_init.cbEngineInit();
					}
				} catch(const std::exception& e) {
					std::cout << e.what() << std::endl;
				}
			}
			return true;
		}
	}
}

