#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"

DEF_LUAIMPLEMENT_PTR(rs::LSysFunc, LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep)(loadClass))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ObjMgr, Object, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_Scene, U_Scene, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR(rs::SceneMgr, SceneMgr, NOTHING, (isEmpty)(getTop)(getScene)(setPushScene)(setPopScene))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::IGLTexture, IGLTexture, NOTHING, (getResourceName)(getTextureID))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, rs::UpdGroup, UpdGroup, "Object", NOTHING, (addObj)(remObj)(getName))
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_UpdGroup, U_UpdGroup, "UpdGroup", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, rs::DrawGroup, DrawGroup, "Object", NOTHING, (addObj)(remObj))
DEF_LUAIMPLEMENT_HDL(ObjMgr, rs::U_DrawGroup, U_DrawGroup, "DrawGroup", NOTHING, NOTHING, (const SortAlgList&)(bool))

namespace rs {
	void LuaImport::RegisterRSClass(LuaState& lsc) {
		LuaImport::RegisterUpdaterObject(lsc);
		LuaImport::RegisterClass<LSysFunc>(lsc);
		LuaImport::RegisterClass<IGLTexture>(lsc);
		LuaImport::RegisterClass<U_Scene>(lsc);
		LuaImport::RegisterClass<UpdGroup>(lsc);
		LuaImport::RegisterClass<U_UpdGroup>(lsc);
		LuaImport::RegisterClass<DrawGroup>(lsc);
		LuaImport::RegisterClass<U_DrawGroup>(lsc);
		LuaImport::ImportClass(lsc, "System", "scene", &mgr_scene);
		LuaImport::ImportClass(lsc, "System", "lsys", &mgr_lsys);
	}
}
