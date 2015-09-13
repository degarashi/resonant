#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "spinner/pose.hpp"

DEF_LUAIMPLEMENT_PTR(spn::DegF, Degree, NOTHING,
		(set)(get)(single)(rangeValue)(range)(luaAddD)(luaAddR)(luaSubD)(luaSubR)(luaMulF)(luaDivF)(luaInvert)(luaToDegree)(luaToRadian)(luaLessthan)(luaLessequal)(luaEqual)(luaToString), (float))
DEF_LUAIMPLEMENT_PTR(spn::RadF, Radian, NOTHING,
		(set)(get)(single)(rangeValue)(range)(luaAddD)(luaAddR)(luaSubD)(luaSubR)(luaMulF)(luaDivF)(luaInvert)(luaToDegree)(luaToRadian)(luaLessthan)(luaLessequal)(luaEqual)(luaToString), (float))
DEF_LUAIMPLEMENT_PTR(spn::Vec2, Vec2, (x)(y),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(ccw)(cw)(asVec3)
		(Ccw)(Cw), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Vec3, Vec3, (x)(y)(z),
		(addV)(subV)(mulF)(mulM)(divF)(modV)(mulQ)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(verticalVector)(asVec4)(asVec2)(lua_planeDivide), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Vec4, Vec4, (x)(y)(z)(w),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(asVec3)(asVec3Coord), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat22, Mat22, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Rotation), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat33, Mat33, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Translation)(RotationX)(RotationY)(RotationZ)(RotationAxis), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Mat44, Mat44, NOTHING,
		(identity)(transposition)(lua_invert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Translation)(LookAtLH)(LookDirLH)(LookAtRH)(LookDirRH)(RotationX)(RotationY)(RotationZ)(RotationAxis)(PerspectiveFovLH)(PerspectiveFovRH), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Quat, Quat, (x)(y)(z)(w),
		(rotationX)(rotationY)(rotationZ)(identity)(normalization)(conjugation)(inverse)(angle)(length)
		(addQ)(subQ)(mulQ)(mulF)(divF)(equal)(toString)
		(getVector)(getAxis)(getXAxis)(getXAxisInv)(getYAxis)(getYAxisInv)(getZAxis)(getZAxisInv)(getRight)(getUp)(getDir)
		(dot)(slerp)(distance)(asMat33)(asMat44), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Pose2D, Pose2D, NOTHING,
		(getOffset)(getScale)(getAngle)(getUp)(getRight)(lua_getToWorld)(lua_getToLocal)
		(setScale)(setAngle)(setOffset)(setUp)
		(identity)(moveUp)(moveDown)(moveLeft)(moveRight)(lerp)(equal)(toString), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Pose3D, Pose3D, NOTHING,
		(getOffset)(getRot)(getScale)(lua_getToWorld)(lua_getToLocal)(getUp)(getRight)(getDir)
		(setAll)(setScale)(setRot)(addAxisRot)(setOffset)(addOffset)
		(identity)(moveFwd2D)(moveSide2D)(moveFwd3D)(moveSide3D)(turnAxis)(turnYPR)(addRot)(lerpTurn)(adjustNoRoll)(lerp)(equal)(toString), NOTHING)
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
		LuaImport::RegisterClass<spn::Pose2D>(lsc);
		LuaImport::RegisterClass<spn::Pose3D>(lsc);
		LuaImport::RegisterClass<spn::DegF>(lsc);
		LuaImport::RegisterClass<spn::RadF>(lsc);
		LuaImport::ImportClass(lsc, "System", "scene", &mgr_scene);
		LuaImport::ImportClass(lsc, "System", "lsys", &mgr_lsys);
		LuaImport::ImportClass(lsc, "System", "glres", &mgr_gl);
		LuaImport::ImportClass(lsc, "System", "sound", &mgr_sound);
		LuaImport::ImportClass(lsc, "System", "input", &mgr_input);
	}
}
