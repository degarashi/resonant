#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"
#include "input.hpp"
#include "sound.hpp"

DEF_LUAIMPLEMENT_PTR(spn::Vec2, Vec2, (x)(y),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Vec3, Vec3, (x)(y)(z),
		(addV)(subV)(mulF)(mulM)(divF)(modV)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Vec4, Vec4, (x)(y)(z)(w),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat22, Mat22, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat33, Mat33, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat44, Mat44, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF), NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::LSysFunc, LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep)(loadClass))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ObjMgr, Object, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_Scene, U_Scene, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::SceneMgr, SceneMgr, NOTHING, (isEmpty)(getTop)(getScene)(setPushScene)(setPopScene))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::IGLTexture, IGLTexture, NOTHING, (getResourceName)(getTextureID))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, rs::UpdGroup, UpdGroup, "Object", NOTHING, (addObj)(remObj)(getName))
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_UpdGroup, U_UpdGroup, "UpdGroup", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, rs::DrawGroup, DrawGroup, "Object", NOTHING, (addObj)(remObj))
DEF_LUAIMPLEMENT_HDL(ObjMgr, rs::U_DrawGroup, U_DrawGroup, "DrawGroup", NOTHING, NOTHING, (const SortAlgList&)(bool))
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::GLRes, GLRes, NOTHING, (loadTexture)(loadCubeTexture)(createTexture)(makeFBuffer)(makeRBuffer))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLRBuffer, GLRBuffer, NOTHING, (getBufferID)(getWidth)(getHeight))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLFBuffer, GLFBuffer, NOTHING, (attachRBuffer)(attachTexture)(detach))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ABufMgr, rs::ABuffer, ABuffer, NOTHING, (isStreaming))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SGroupMgr, rs::AGroup, AGroup, NOTHING, (pause)(resume)(clear)(play)(fadeIn)(fadeInOut)(getChannels)(getIdleChannels)(getPlayingChannels))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SSrcMgr, rs::ASource, ASource, NOTHING, (play)(pause)(stop)(setFadeTo)(setFadeOut)(setBuffer)(getLooping)(getNLoop)(setPitch)(setGain)(setRelativeMode))
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::SoundMgr, SoundMgr, NOTHING, (loadWaveBatch)(loadOggBatch)(loadOggStream)(createSourceGroup)(createSource))

DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::InputMgr, InputMgr, NOTHING, (makeAction)(addAction)(remAction))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::detail::ActMgr, rs::detail::Action, Action, NOTHING, (isKeyPressed)(isKeyReleased)(isKeyPressing)(addLink)(remLink)(getState)(getValue))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::InputMgrBase, rs::IInput, IInput, NOTHING, (name)(getButton)(getAxis)(getHat)(numButtons)(numAxes)(numHats)(setDeadZone)(setMouseMode)(getMouseMode))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Keyboard, Keyboard, "IInput", NOTHING, (OpenKeyboard))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Mouse, Mouse, "IInput", NOTHING, (OpenMouse)(NumMouse))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Joypad, Joypad, "IInput", NOTHING, (OpenJoypad)(NumJoypad))

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
		LuaImport::RegisterClass<detail::Action>(lsc);
		LuaImport::RegisterClass<GLRBuffer>(lsc);
		LuaImport::RegisterClass<GLFBuffer>(lsc);
		LuaImport::RegisterClass<ABuffer>(lsc);
		LuaImport::RegisterClass<AGroup>(lsc);
		LuaImport::RegisterClass<ASource>(lsc);
		LuaImport::RegisterClass<IInput>(lsc);
		LuaImport::RegisterClass<Keyboard>(lsc);
		LuaImport::RegisterClass<Mouse>(lsc);
		LuaImport::RegisterClass<Joypad>(lsc);
		LuaImport::RegisterClass<spn::Vec2>(lsc);
		LuaImport::RegisterClass<spn::Vec3>(lsc);
		LuaImport::RegisterClass<spn::Vec4>(lsc);
		LuaImport::RegisterClass<spn::Mat22>(lsc);
		LuaImport::RegisterClass<spn::Mat33>(lsc);
		LuaImport::RegisterClass<spn::Mat44>(lsc);
		LuaImport::ImportClass(lsc, "System", "scene", &mgr_scene);
		LuaImport::ImportClass(lsc, "System", "lsys", &mgr_lsys);
		LuaImport::ImportClass(lsc, "System", "glres", &mgr_gl);
		LuaImport::ImportClass(lsc, "System", "sound", &mgr_sound);
		LuaImport::ImportClass(lsc, "System", "input", &mgr_input);
	}
}
