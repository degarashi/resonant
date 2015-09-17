#include "glresource.hpp"
#include "luaw.hpp"
#include "lsys.hpp"
#include "scene.hpp"
#include "updater.hpp"
#include "updater_lua.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "spinner/pose.hpp"
#include "spinner/plane.hpp"
#include "camera.hpp"
#include "camera2d.hpp"
#include "spinner/random.hpp"
#include "util/screen.hpp"

DEF_LUAIMPLEMENT_PTR(rs::draw::ClearParam, ClearParam, (color)(depth)(stencil), NOTHING, (spn::Optional<spn::Vec4>)(spn::Optional<float>)(spn::Optional<uint32_t>))
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::util::FBClear, FBClear, "Object", NOTHING, NOTHING, (rs::Priority)(const rs::draw::ClearParam&))
using ClearParam_OP = spn::Optional<rs::draw::ClearParam>;
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::util::FBSwitch, FBSwitch, "Object", NOTHING, (setClearParam), (rs::Priority)(HFb)(const ClearParam_OP&))

DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::MTRandom, Random, NOTHING, (luaGetUniform<float>)(luaGetUniform<int>))
DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::MTRandomMgr, RandomMgr, NOTHING, (initEngine)(removeEngine)(get))
DEF_LUAIMPLEMENT_PTR(spn::DegF, Degree, NOTHING,
		(set)(get)(single)(rangeValue)(range)(luaAddD)(luaAddR)(luaSubD)(luaSubR)(luaMulF)(luaDivF)(luaInvert)(luaToDegree)(luaToRadian)(luaLessthan)(luaLessequal)(luaEqual)(luaToString), (float))
DEF_LUAIMPLEMENT_PTR(spn::RadF, Radian, NOTHING,
		(set)(get)(single)(rangeValue)(range)(luaAddD)(luaAddR)(luaSubD)(luaSubR)(luaMulF)(luaDivF)(luaInvert)(luaToDegree)(luaToRadian)(luaLessthan)(luaLessequal)(luaEqual)(luaToString), (float))
DEF_LUAIMPLEMENT_PTR(spn::Vec2, Vec2, (x)(y),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(ccw)(cw)(asVec3)
		(Ccw)(Cw)
		(luaRandom)(luaRandomWithLength)(luaRandomWithAbs), (float)(float))
DEF_LUAIMPLEMENT_PTR(spn::Vec3, Vec3, (x)(y)(z),
		(addV)(subV)(mulF)(mulM)(divF)(modV)(mulQ)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(verticalVector)(asVec4)(asVec2)(luaPlaneDivide)
		(luaRandom)(luaRandomWithLength)(luaRandomWithAbs), (float)(float)(float))
DEF_LUAIMPLEMENT_PTR(spn::Vec4, Vec4, (x)(y)(z)(w),
		(addV)(subV)(mulF)(mulM)(divF)(invert)(equal)(toString)
		(dot<false>)(sum)(distance<false>)(getMin<false>)(selectMin<false>)(getMax<false>)(selectMax<false>)
		(normalization)(length)(saturation)(l_intp<false>)(asVec3)(asVec3Coord)
		(luaRandom)(luaRandomWithLength)(luaRandomWithAbs), (float)(float)(float)(float))
DEF_LUAIMPLEMENT_DERIVED(spn::AVec2, spn::Vec2)
DEF_LUAIMPLEMENT_DERIVED(spn::AVec3, spn::Vec3)
DEF_LUAIMPLEMENT_DERIVED(spn::AVec4, spn::Vec4)

DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::Mat22, Mat22, NOTHING,
		(identity)(transposition)(luaInvert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Rotation))
DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::Mat33, Mat33, NOTHING,
		(identity)(transposition)(luaInvert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Translation)(RotationX)(RotationY)(RotationZ)(RotationAxis))
DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::Mat44, Mat44, NOTHING,
		(identity)(transposition)(luaInvert)(calcDeterminant)
		(addF)(addM)(subF)(subM)(mulF)(mulM)(mulV)(divF)
		(Scaling)(Translation)(LookAtLH)(LookDirLH)(LookAtRH)(LookDirRH)(RotationX)(RotationY)(RotationZ)(RotationAxis)(PerspectiveFovLH)(PerspectiveFovRH))
DEF_LUAIMPLEMENT_DERIVED(spn::AMat22, spn::Mat22)
DEF_LUAIMPLEMENT_DERIVED(spn::AMat33, spn::Mat33)
DEF_LUAIMPLEMENT_DERIVED(spn::AMat44, spn::Mat44)

DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::Quat, Quat, (x)(y)(z)(w),
		(rotationX)(rotationY)(rotationZ)(identity)(normalization)(conjugation)(inverse)(angle)(length)
		(addQ)(subQ)(mulQ)(mulF)(divF)(equal)(toString)
		(getVector)(getAxis)(getXAxis)(getXAxisInv)(getYAxis)(getYAxisInv)(getZAxis)(getZAxisInv)(getRight)(getUp)(getDir)
		(dot)(slerp)(distance)(asMat33)(asMat44)
		(FromAxisF)(FromAxis)(FromMatAxis)(RotationYPR)(RotationX)(RotationY)(RotationZ)(LookAt)(SetLookAt)(Lua_FromMat33)(Lua_FromMat44)(Lua_Rotation)(Lua_RotationFromTo))
DEF_LUAIMPLEMENT_DERIVED(spn::AQuat, spn::Quat)

DEF_LUAIMPLEMENT_PTR_NOCTOR(spn::Plane, Plane, (a)(b)(c)(d),
		(dot)(move)(getNormal)(placeOnPlane)(placeOnPlaneDirDist)(getOrigin)
		(mulM)(equal)(toString)
		(FromPtDir)(FromPts)(ChokePoint)(CrossLine))
DEF_LUAIMPLEMENT_DERIVED(spn::APlane, spn::Plane)

DEF_LUAIMPLEMENT_PTR(spn::Pose2D, Pose2D, NOTHING,
		(getOffset)(getScale)(getAngle)(getUp)(getRight)(luaGetToWorld)(luaGetToLocal)
		(setScale)(setAngle)(setOffset)(setUp)
		(identity)(moveUp)(moveDown)(moveLeft)(moveRight)(lerp)(equal)(toString), NOTHING)
DEF_LUAIMPLEMENT_PTR(spn::Pose3D, Pose3D, NOTHING,
		(getOffset)(getRot)(getScale)(luaGetToWorld)(luaGetToLocal)(getUp)(getRight)(getDir)
		(setAll)(setScale)(setRot)(addAxisRot)(setOffset)(addOffset)
		(identity)(moveFwd2D)(moveSide2D)(moveFwd3D)(moveSide3D)(turnAxis)(turnYPR)(addRot)(lerpTurn)(adjustNoRoll)(lerp)(equal)(toString), NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::LSysFunc, LSysFunc, NOTHING, (loadResource)(loadResources)(loadResourcesASync)(queryProgress)(getResult)(getNTask)(sleep)(loadClass))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ObjMgr, Object, Object, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_Scene, U_Scene, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::SceneMgr, SceneMgr, NOTHING, (isEmpty)(getTop)(getScene)(setPushScene)(setPopScene))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::IGLTexture, IGLTexture, NOTHING, (getResourceName)(getTextureID))
DEF_LUAIMPLEMENT_HDL_NOCTOR(ObjMgr, rs::UpdGroup, UpdGroup, "Object", NOTHING, (addObj)(addObjPriority)(remObj)(getName)(getPriority)(clear))
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, rs::U_UpdGroup, U_UpdGroup, "UpdGroup", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, rs::DrawGroup, DrawGroup, "Object", NOTHING, (addObj)(remObj)(setSortAlgorithmId)(getName)(setPriority))
DEF_LUAIMPLEMENT_HDL(ObjMgr, rs::U_DrawGroup, U_DrawGroup, "DrawGroup", NOTHING, NOTHING, (const SortAlgList&)(bool))
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::GLRes, GLRes, NOTHING, (loadTexture)(loadCubeTexture)(createTexture)(makeFBuffer)(makeRBuffer))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLRBuffer, GLRBuffer, NOTHING, (getBufferID)(getWidth)(getHeight))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::GLRes, rs::GLFBuffer, GLFBuffer, NOTHING, (attachRBuffer)(attachTexture)(detach))

DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::ABufMgr, rs::ABuffer, ABuffer, NOTHING, (isStreaming))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SGroupMgr, rs::AGroup, AGroup, NOTHING, (pause)(resume)(clear)(play)(fadeIn)(fadeInOut)(getChannels)(getIdleChannels)(getPlayingChannels))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::SSrcMgr, rs::ASource, ASource, NOTHING, (play)(pause)(stop)(setFadeTo)(setFadeOut)(setBuffer)(getLooping)(getNLoop)(setPitch)(setGain)(setRelativeMode))
DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::SoundMgr, SoundMgr, NOTHING, (loadWaveBatch)(loadOggBatch)(loadOggStream)(createSourceGroup)(createSource))

DEF_LUAIMPLEMENT_PTR_NOCTOR(rs::InputMgr, InputMgr, NOTHING, (makeAction)(addAction)(remAction))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::detail::ActMgr, rs::detail::Action, Action, NOTHING, (isKeyPressed)(isKeyReleased)(isKeyPressing)(addLink)(remLink)(getState)(getValue)
	(getKeyValueSimplified)(linkButtonAsAxis))
DEF_LUAIMPLEMENT_HDL_NOBASE_NOCTOR(rs::InputMgrBase, rs::IInput, IInput, NOTHING,
	(name)(getButton)(getAxis)(getHat)(numButtons)(numAxes)(numHats)(setDeadZone)(setMouseMode)(getMouseMode))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Keyboard, Keyboard, "IInput", NOTHING, (OpenKeyboard))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Mouse, Mouse, "IInput", NOTHING, (OpenMouse)(NumMouse))
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::InputMgrBase, rs::Joypad, Joypad, "IInput", NOTHING, (OpenJoypad)(NumJoypad))

DEF_LUAIMPLEMENT_HDL_NOBASE(rs::Camera3DMgr, rs::Camera3D, Camera3D, NOTHING,
		(setPose<spn::Pose3D>)(setFov<spn::RadF>)(setAspect<float>)(setNearZ<float>)(setFarZ<float>)(setZPlane)
		(getPose)(getFov)(getAspect)(getNearZ)(getFarZ)
		(unproject)(unprojectVec)(vp2wp), NOTHING)
DEF_LUAIMPLEMENT_HDL_NOBASE(rs::Camera2DMgr, rs::Camera2D, Camera2D, NOTHING,
		(setPose<spn::Pose2D>)(setAspectRatio<float>)
		(getPose)(getAspectRatio)
		(vp2w)(v2w), NOTHING)

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
		LuaImport::RegisterClass<spn::Quat>(lsc);
		LuaImport::RegisterClass<spn::Plane>(lsc);
		LuaImport::RegisterClass<spn::DegF>(lsc);
		LuaImport::RegisterClass<spn::RadF>(lsc);
		LuaImport::RegisterClass<Camera2D>(lsc);
		LuaImport::RegisterClass<Camera3D>(lsc);
		LuaImport::RegisterClass<spn::MTRandom>(lsc);
		LuaImport::RegisterClass<spn::MTRandomMgr>(lsc);
		LuaImport::RegisterClass<draw::ClearParam>(lsc);
		LuaImport::RegisterClass<util::FBSwitch>(lsc);
		LuaImport::RegisterClass<util::FBClear>(lsc);
		LuaImport::ImportClass(lsc, "System", "scene", &mgr_scene);
		LuaImport::ImportClass(lsc, "System", "lsys", &mgr_lsys);
		LuaImport::ImportClass(lsc, "System", "glres", &mgr_gl);
		LuaImport::ImportClass(lsc, "System", "sound", &mgr_sound);
		LuaImport::ImportClass(lsc, "System", "input", &mgr_input);
		LuaImport::ImportClass(lsc, "System", "random", &mgr_random);
	}
}
