#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"
#include "sound.hpp"

DEF_LUAIMPLEMENT_PTR(rs::LSysFunc, LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep)(loadClass))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ObjMgr, Object, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_Scene, U_Scene, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR(rs::SceneMgr, SceneMgr, NOTHING, (isEmpty)(getTop)(getScene)(setPushScene)(setPopScene))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::IGLTexture, IGLTexture, NOTHING, (getResourceName)(getTextureID))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, rs::UpdGroup, UpdGroup, "Object", NOTHING, (addObj)(remObj)(getName))
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_UpdGroup, U_UpdGroup, "UpdGroup", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, rs::DrawGroup, DrawGroup, "Object", NOTHING, (addObj)(remObj))
DEF_LUAIMPLEMENT_HDL(ObjMgr, rs::U_DrawGroup, U_DrawGroup, "DrawGroup", NOTHING, NOTHING, (const SortAlgList&)(bool))
DEF_LUAIMPLEMENT_PTR(rs::GLRes, GLRes, NOTHING, (loadTexture)(loadCubeTexture)(createTexture)(makeFBuffer)(makeRBuffer))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLRBuffer, GLRBuffer, NOTHING, (getBufferID)(getWidth)(getHeight))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLFBuffer, GLFBuffer, NOTHING, (attachRBuffer)(attachTexture)(detach))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ABufMgr, rs::ABuffer, ABuffer, NOTHING, (isStreaming))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SGroupMgr, rs::AGroup, AGroup, NOTHING, (pause)(resume)(clear)(play)(fadeIn)(fadeInOut)(getChannels)(getIdleChannels)(getPlayingChannels))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SSrcMgr, rs::ASource, ASource, NOTHING, (play)(pause)(stop)(setFadeTo)(setFadeOut)(setBuffer)(getLooping)(getNLoop)(setPitch)(setGain)(setRelativeMode))
DEF_LUAIMPLEMENT_PTR(rs::SoundMgr, SoundMgr, NOTHING, (loadWaveBatch)(loadOggBatch)(loadOggStream)(createSourceGroup)(createSource))

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
		LuaImport::RegisterClass<GLRBuffer>(lsc);
		LuaImport::RegisterClass<GLFBuffer>(lsc);
		LuaImport::RegisterClass<ABuffer>(lsc);
		LuaImport::RegisterClass<AGroup>(lsc);
		LuaImport::RegisterClass<ASource>(lsc);
		LuaImport::ImportClass(lsc, "System", "scene", &mgr_scene);
		LuaImport::ImportClass(lsc, "System", "lsys", &mgr_lsys);
		LuaImport::ImportClass(lsc, "System", "glres", &mgr_gl);
		LuaImport::ImportClass(lsc, "System", "sound", &mgr_sound);
	}
}
