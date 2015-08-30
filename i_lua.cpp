#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"

DEF_LUAIMPLEMENT_PTR(LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(ObjMgr, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(::rs::ObjMgr, U_Scene, "Object", NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR(SceneMgr, NOTHING, (isEmpty)(getTop))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(GLRes, IGLTexture, NOTHING, (getResourceName)(getTextureID))
