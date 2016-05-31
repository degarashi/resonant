#include "test.hpp"
#include "engine.hpp"
#include "blureffect.hpp"
#include "twophaseblur.hpp"
#include "gaussblur.hpp"
#include "bilateralblur.hpp"
#include "sprite.hpp"
#include "sprite3d.hpp"
#include "primitive.hpp"
#include "infoshow.hpp"
#include "fpscamera.hpp"
#include "dlight.hpp"
#include "tile.hpp"
#include "stile.hpp"
#include "skydome.hpp"
#include "tonemap.hpp"
#include "clipmap.hpp"
#include "colview.hpp"
#include "tweak/tweak.hpp"
#include "../luaw.hpp"
#include "../util/profileshow.hpp"
#include "../updater_lua.hpp"
#include "../camera.hpp"
#include "../input.hpp"

rs::CCoreID GetCID() {
	return mgr_text.makeCoreID(g_fontName, rs::CCoreID(0, 5, rs::CCoreID::CharFlag_AA, false, 0, rs::CCoreID::SizeType_Point));
}
class U_ProfileShow : public rs::util::ProfileShow {
	public:
		U_ProfileShow(rs::HDGroup hDg, rs::Priority uprio, rs::Priority dprio):
			rs::util::ProfileShow(TextShow::T_Text, T_Rect,
				GetCID(), hDg, uprio, dprio) {}
};
DEF_LUAIMPORT(U_ProfileShow)

DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, BilateralBlur, BilateralBlur, "TwoPhaseBlur",
	NOTHING,
	(setGDispersion<float>)
	(setBDispersion<float>),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, BlurEffect, BlurEffect, "Object",
	NOTHING,
	(setAlpha)(setDiffuse)(setRect),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ClipmapObj, ClipmapObj, "DrawableObj",
	NOTHING,
	(setRayleighCoeff)(setMieCoeff)(setLightDir)(setLightColor)(setLightPower)(setDivide)
	(setGridSize)(setCamera)(getCache)(getNormalCache)(getDrawCount)(setGridSize)(setDiffuseSize)
	(setPNElevation)(setWaveElevation),
	(int)(int)(int)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(Clipmap::DrawCount, DrawCount,
	(draw)(not_draw),
	NOTHING
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ColBoxObj, ColBoxObj, "DrawableObj",
	NOTHING,
	(setAlpha)
	(setColor)
	(setOffset)
	(setRot)
	(setScale),
	NOTHING
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(DLight, DLight,
	NOTHING,
	(setPosition<const spn::Vec3&>)
	(setColor<const spn::Vec3&>)
	(setCoeff<const spn::Vec2&>)
	(setDirection<const spn::Vec3&>)
	(setDepthRange<const spn::Vec2&>)
	(setEyeCamera<rs::HCam>)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(Engine, Engine,
	NOTHING,
	(setLineLength<float>)
	(setLightDepthSize<const spn::Size&>)
	(setDispersion)
	(getDLScene)
	(getDrawScene)
	(getCubeScene)
	(addSceneObject)
	(remSceneObject)
	(setSceneGroup)
	(setOutputFramebuffer)
	(clearScene)
	(makeLight)
	(remLight)
	(getLight)
	(setOffset)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, FPSCamera, FPSCamera, "FSMachine", NOTHING, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, U_ProfileShow, U_ProfileShow, "Object",
	NOTHING,
	(setOffset),
	(rs::HDGroup)(rs::Priority)(rs::Priority)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(GlobalValue, GlobalValue,
	(hlCam)(random)(hlIk)(hlIm)
	(actQuit)(actAx)(actAy)(actMoveX)(actMoveY)
	(actPress)(actReset)(actCube)(actSound)(actSprite)(actSpriteD)
	(actNumber0)(actNumber1)(actNumber2)(actNumber3)(actNumber4),
	NOTHING
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(IClipSource_SP, IClipSource_SP, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(HashVec_SP, HashVec_SP, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_PTR_NOCTOR(Hash_SP, Hash_SP, NOTHING, NOTHING)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, GaussBlur, GaussBlur, "TwoPhaseBlur",
	NOTHING,
	(setDispersion<float>),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TextShow, TextShow, "Object",
	NOTHING,
	(setOffset)(setText)(getPriority)(setBGDepth)(setBGColor)(setBGAlpha),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, InfoShow, InfoShow, "TextShow",
	NOTHING,
	NOTHING,
	(rs::HDGroup)(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, OffsetAdd, OffsetAdd, "DrawableObj",
	NOTHING,
	(setAdd1)(setAdd2)(setAdd3)(setAlpha)(setOffset)(setRatio),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, PrimitiveObj, PrimitiveObj, "DrawableObj",
	NOTHING,
	(showNormals)
	(advance)
	(setOffset),
	(float)(rs::HTex)(rs::HTex)(Primitive::Type::E)(bool)(bool)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, Reduction, Reduction, "Object",
	NOTHING,
	(setSource)(getResult),
	(rs::Priority)(bool)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SkyDome, SkyDome, "DrawableObj",
	NOTHING,
	(setScale)
	(setDivide)
	(setRayleighCoeff)
	(setMieCoeff)
	(setLightPower)
	(setLightDir)
	(setLightColor),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, SpriteObj, SpriteObj, "DrawableObj",
	NOTHING,
	(setOffset)
	(setScale)
	(setAngle)
	(setZOffset)
	(setZRange)
	(setAlpha),
	(rs::HTex)(float)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, BoundingSprite, BoundingSprite, "DrawableObj",
	NOTHING,
	(setScale),
	(rs::HTex)(const spn::Vec2&)(const spn::Vec2&)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, PointSprite3D, PointSprite3D, "DrawableObj",
	NOTHING,
	(setOffset)
	(setScale)
	(setAlpha),
	(rs::HTex)(const spn::Vec3&)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, STileField, STileField, "TileFieldBase",
	NOTHING,
	(setRayleighCoeff)(setMieCoeff)(setLightDir)(setLightColor)(setLightPower)(setDivide)
	(setViewDistanceCoeff),
	(spn::MTRandom&)(int)(int)(float)(float)(float)(float)(float)
)
DEF_LUAIMPLEMENT_HDL_NOCTOR(rs::ObjMgr, TileFieldBase, TileFieldBase, "DrawableObj",
	NOTHING,
	(setTexture)
	(setTextureRepeat)
	(setViewPos)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TileField, TileField, "TileFieldBase",
	NOTHING,
	(setViewDistanceCoeff),
	(spn::MTRandom&)(int)(int)(float)(float)(float)(float)(float)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, ToneMap, ToneMap, "DrawableObj",
	NOTHING,
	(setSource<rs::HTex>)
	(getResult)(getShrink0)(getShrinkA),
	(rs::Priority)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, tweak::Tweak, Tweak, "DrawableObj",
	NOTHING,
	(saveAll)(save)
	(setEntryFromTable)(setEntryFromFile)(setEntryDefault)
	(setDrawPriority)
	(setCursorAxis)(setIncrementAxis)(setSwitchButton)
	(makeGroup)(makeEntry)(setValue)(increment)(remove)(removeObj)
	(setFontSize)(setOffset)
	(insertNext)(insertChild)
	(expand)(fold)(up)(down)(next)(prev)(setCursor)(getCursor)(getRoot),
	(const std::string&)(int)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(tweak::INode, INode,
	NOTHING,
	(sortAlphabetically)
)
DEF_LUAIMPLEMENT_PTR_NOCTOR(tweak::INode::SP, INodeSP,
	NOTHING,
	(use_count)(get)
)
DEF_LUAIMPLEMENT_HDL(rs::ObjMgr, TwoPhaseBlur, TwoPhaseBlur, "Object",
	NOTHING,
	(setSource<rs::HTex>)
	(setDest<rs::HTex>)
	(setTmpFormat<int>),
	(rs::Priority)
)

void RegisterRSTestClass(rs::LuaState& lsc) {
	auto& gv = *tls_gvalue;
	rs::LuaImport::RegisterClass<BlurEffect>(lsc);
	rs::LuaImport::RegisterClass<TwoPhaseBlur>(lsc);
	rs::LuaImport::RegisterClass<GaussBlur>(lsc);
	rs::LuaImport::RegisterClass<BilateralBlur>(lsc);
	rs::LuaImport::RegisterClass<BoundingSprite>(lsc);
	rs::LuaImport::RegisterClass<SpriteObj>(lsc);
	rs::LuaImport::RegisterClass<PointSprite3D>(lsc);
	rs::LuaImport::RegisterClass<PrimitiveObj>(lsc);
	rs::LuaImport::RegisterClass<TextShow>(lsc);
	rs::LuaImport::RegisterClass<InfoShow>(lsc);
	rs::LuaImport::RegisterClass<U_ProfileShow>(lsc);
	rs::LuaImport::RegisterClass<GlobalValue>(lsc);
	rs::LuaImport::ImportClass(lsc, "Global", "cpp", &gv);
	rs::LuaImport::RegisterClass<FPSCamera>(lsc);
	rs::LuaImport::RegisterClass<DLight>(lsc);
	rs::LuaImport::RegisterClass<TileFieldBase>(lsc);
	rs::LuaImport::RegisterClass<TileField>(lsc);
	rs::LuaImport::RegisterClass<STileField>(lsc);
	rs::LuaImport::RegisterClass<SkyDome>(lsc);
	rs::LuaImport::RegisterClass<ToneMap>(lsc);
	rs::LuaImport::RegisterClass<ClipmapObj>(lsc);
	rs::LuaImport::RegisterClass<ColBoxObj>(lsc);
	rs::LuaImport::RegisterClass<Hash_SP>(lsc);
	rs::LuaImport::RegisterClass<HashVec_SP>(lsc);
	rs::LuaImport::RegisterClass<IClipSource_SP>(lsc);
	rs::LuaImport::RegisterClass<tweak::Tweak>(lsc);
	rs::LuaImport::RegisterClass<tweak::INode::SP>(lsc);
	rs::LuaImport::RegisterClass<tweak::INode>(lsc);
	rs::LuaImport::RegisterFunction(lsc, "MakeCS_PN", &ClipPNSource::Create);
	rs::LuaImport::RegisterFunction(lsc, "MakeCS_Tex", &ClipTexSource::Create);
	rs::LuaImport::RegisterFunction(lsc, "MakeHash2D", &Hash2D::Create);
	rs::LuaImport::RegisterFunction(lsc, "MakeHash", &ClipHash::Create);
	rs::LuaImport::RegisterFunction(lsc, "MakeHash_Mod", &ClipHashMod::Create);
	rs::LuaImport::RegisterFunction(lsc, "MakeHash_Vec", &ClipHashV::Create);
}
