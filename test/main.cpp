#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif
#include "test.hpp"
#include "../adaptsdl.hpp"
#include "../gameloophelper.hpp"
#include "../camera.hpp"
#include "../input.hpp"
#include "scene.hpp"
#include "engine.hpp"
#include "fpscamera.hpp"

#include "../updater.hpp"
#include "../updater_lua.hpp"
#include "infoshow.hpp"
#include "../util/profileshow.hpp"
#include "cube.hpp"
#include "sprite.hpp"
#include "sprite3d.hpp"
#include "blureffect.hpp"

rs::CCoreID GetCID() {
	return mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, 5, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Point));
}
class U_ProfileShow : public rs::util::ProfileShow {
	public:
		U_ProfileShow(rs::HDGroup hDg, rs::Priority uprio, rs::Priority dprio):
			rs::util::ProfileShow(InfoShow::T_Info, T_Rect,
				GetCID(), hDg, uprio, dprio) {}
};
DEF_LUAIMPORT(U_ProfileShow)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, U_ProfileShow, U_ProfileShow, "Object", NOTHING, (setOffset), (rs::HDGroup)(rs::Priority)(rs::Priority))

DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, FPSCamera, FPSCamera, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(GlobalValue, GlobalValue,
			(hlCam)(random)(hlIk)(hlIm)
			(actQuit)(actAx)(actAy)(actMoveX)(actMoveY)
			(actPress)(actReset)(actCube)(actSound)(actSprite)(actSpriteD)
			(actNumber0)(actNumber1)(actNumber2)(actNumber3)(actNumber4), NOTHING)

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
int main(int argc, char **argv) {
	// 第一引数にpathlistファイルの指定が必須
	if(argc <= 1) {
		LogOutput("usage: rse_demo pathlist_file");
		return 0;
	}
	rs::GLoopInitializer init;
	init.cbInit = [](){
		auto lkb = sharedbase.lockR();
		tls_gvalue = GlobalValue();
		auto& gv = *tls_gvalue;

		rs::LuaImport::RegisterClass<BlurEffect>(*lkb->spLua);
		rs::LuaImport::RegisterClass<U_BoundingSprite>(*lkb->spLua);
		rs::LuaImport::RegisterClass<SpriteObj>(*lkb->spLua);
		rs::LuaImport::RegisterClass<PointSprite3D>(*lkb->spLua);
		rs::LuaImport::RegisterClass<CubeObj>(*lkb->spLua);
		rs::LuaImport::RegisterClass<InfoShow>(*lkb->spLua);
		rs::LuaImport::RegisterClass<U_ProfileShow>(*lkb->spLua);
		rs::LuaImport::RegisterClass<GlobalValue>(*lkb->spLua);
		rs::LuaImport::ImportClass(*lkb->spLua, "Global", "cpp", &gv);
		rs::LuaImport::RegisterClass<FPSCamera>(*lkb->spLua);

		// init Random
		{
			mgr_random.initEngine(RandomId);
			gv.random = mgr_random.get(RandomId);
		}
	};
	init.cbEngineInit = [](){
		auto& gv = *tls_gvalue;
		// init Camera
		{
			gv.hlCam = mgr_cam.emplace();
			auto& cd = gv.hlCam.ref();
			auto& ps = cd.refPose();
			ps.setOffset({0,0,-3});
			cd.setFov(spn::DegF(60));
			cd.setZPlane(0.01f, 500.f);
		}
		auto lkb = sharedbase.lockR();
		// init Effect
		{
			gv.pEngine = static_cast<Engine*>(lkb->hlFx->get());
			gv.pEngine->ref<rs::SystemUniform3D>().setCamera(gv.hlCam);
		}
	};
	init.cbPreTerm = [](){
		tls_gvalue = spn::none;
	};
	return rs::GameloopHelper<Engine, SharedValue, Sc_Dummy>::Run(init, RESOLUTION_X, RESOLUTION_Y, APP_NAME, argv[1]);
}
