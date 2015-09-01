#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"

DEF_LUAIMPLEMENT_PTR(LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep)(loadClass))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(ObjMgr, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, U_Scene, "Object", NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR(SceneMgr, NOTHING, (isEmpty)(getTop)(getTop)(getScene)(setPushScene)(setPopScene))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(GLRes, IGLTexture, NOTHING, (getResourceName)(getTextureID))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, UpdGroup, "Object", NOTHING, (addObj)(remObj))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, DrawGroup, "Object", NOTHING, (addObj)(remObj))

namespace rs {
	void LuaImport::RegisterRSClass(LuaState& lsc) {
		LuaImport::RegisterUpdaterObject(lsc);
		LuaImport::RegisterClass<IGLTexture>(lsc);
		LuaImport::RegisterClass<U_Scene>(lsc);
		LuaImport::RegisterClass<LSysFunc>(lsc);
		LuaImport::ImportClass(lsc, "LSys", &mgr_lsys);
	}
}
