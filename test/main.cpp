#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"
#include "engine.hpp"
#include "../camera.hpp"
#include "../gameloophelper.hpp"
#include "../input.hpp"

thread_local GlobalValueOP tls_gvalue;
namespace {
	constexpr int RandomId = 0x20000000;
}
Engine& CnvToEngine(rs::IEffect& e) {
	return static_cast<Engine&>(e);
}
namespace {
	constexpr int RESOLUTION_X = 1024,
				RESOLUTION_Y = 768;
	constexpr char APP_NAME[] = "rse_test";
}
int main(const int argc, const char **argv) {
	// 第一引数にpathlistファイルの指定が必須
	if(argc <= 1) {
		::spn::Log::Output("usage: rse_demo pathlist_file");
		return 0;
	}
	rs::GLoopInitializer init;
	init.cbInit = [](){
		auto lkb = sharedbase.lockR();
		tls_gvalue = GlobalValue();
		auto& gv = *tls_gvalue;
		RegisterRSTestClass(*lkb->spLua);
		// init Random
		{
			mgr_random.initEngine(RandomId);
			gv.random = mgr_random.get(RandomId);
		}
		// init Camera
		{
			gv.hlCam = mgr_cam.emplace();
			auto& cd = gv.hlCam.ref();
			auto& ps = cd.refPose();
			ps.setOffset({0,0,0});
			ps.setRot(spn::AQuat::LookAt({0,0,1}, {0,1,0}));
			cd.setFov(spn::DegF(60));
			cd.setZPlane(0.01f, 500.f);
			gv.pEngine = static_cast<Engine*>(lkb->hlFx->get());
			gv.pEngine->ref<rs::SystemUniform3D>().setCamera(gv.hlCam);
		}
	};
	init.cbEngineInit = [](){
		auto& gv = *tls_gvalue;
		auto lkb = sharedbase.lockR();
		// init Effect
		{
			gv.pEngine = static_cast<Engine*>(lkb->hlFx->get());
			rs::LuaImport::ImportClass(*lkb->spLua, "Global", "engine", gv.pEngine);
		}
	};
	init.cbPreTerm = [](){
		tls_gvalue = spn::none;
	};
	return rs::GameloopHelper<Engine, SharedValue, Sc_Dummy>::Run(init, RESOLUTION_X, RESOLUTION_Y, APP_NAME, argv[1]);
}
