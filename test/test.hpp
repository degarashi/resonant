#pragma once
#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif

#include "spinner/pose.hpp"
#include "spinner/vector.hpp"
#include "../gameloop.hpp"
#include "../updater.hpp"
#include "../scene.hpp"
#include "../vertex.hpp"
#include "../differential.hpp"
#include "../glx_if.hpp"
#include "../util/dwrapper_calldraw.hpp"
namespace vertex {
	//! 当たり判定確認用頂点
	struct collision {
		spn::Vec3	pos;
		spn::Vec4	color;
	};
	struct colview {
		spn::Vec3	pos;
	};
	//! プリミティブ描画用頂点
	struct prim {
		spn::Vec3	pos;
		spn::Vec2	tex;
		spn::Vec3	normal;
	};
	//! 頂点座標空間付き頂点
	struct prim_tan : prim {
		spn::Vec4	tangent_c;
	};
	//! 法線確認用頂点
	struct line {
		spn::Vec3	pos,
					dir;
		spn::Vec4	color;
	};
	//! スプライト描画用頂点
	struct sprite {
		spn::Vec3	pos;
		spn::Vec2	tex;
	};
	struct tile_0 {
		spn::Vec2	pos,
					tex;
	};
	struct tile_1 {
		spn::Vec3	normal;
		float		height;
	};
	struct stile_1 {
		spn::Vec3	height;	// x=1Level低い高さ, y=本来の高さ, z=補間対象レベル
		spn::Vec3	normalX;
		spn::Vec3	normalY;
	};
}
namespace vdecl {
	struct collision {};
	struct colview {};
	struct prim {};
	struct prim_tan {};
	struct line {};
	struct sprite {};
	struct tile {};
	struct stile {};
}
DefineVDecl(::vdecl::collision)
DefineVDecl(::vdecl::colview)
DefineVDecl(::vdecl::prim)
DefineVDecl(::vdecl::prim_tan)
DefineVDecl(::vdecl::line)
DefineVDecl(::vdecl::sprite)
DefineVDecl(::vdecl::tile)
DefineVDecl(::vdecl::stile)

#include "spinner/random.hpp"
class Engine;
class Sc_Dummy : public rs::Scene<Sc_Dummy> {};
using RandomOP = spn::Optional<spn::MTRandom>;
// ゲーム通しての(MainThread, DrawThread含めた)グローバル変数
// リソースハンドルはメインスレッドで参照する -> メインスレッドハンドラに投げる
struct SharedValue {};
#define sharedv (::rs::GameloopHelper<Engine, SharedValue, Sc_Base>::SharedValueC::_ref())
using GlxId = rs::IEffect::GlxId;
rs::HLDObj MakeFBClear(rs::Priority priority);
extern const rs::IdValue T_Rect;

struct GlobalValue {
	Engine*		pEngine;
	rs::HLInput	hlIk,
				hlIm;
	rs::HLAct	actQuit,
				actAx,
				actAy,
				actMoveX,
				actMoveY,
				actPress,
				actReset,
				actCube,
				actSound,
				actSprite,
				actSpriteD,
				actNumber0,
				actNumber1,
				actNumber2,
				actNumber3,
				actNumber4;
	rs::HLCam	hlCam;
	RandomOP	random;
};
DEF_LUAIMPORT(GlobalValue)
using GlobalValueOP = spn::Optional<GlobalValue>;
extern thread_local GlobalValueOP tls_gvalue;
extern const std::string MSG_GetState;
void RegisterRSTestClass(rs::LuaState& lsc);

class Engine;
Engine& CnvToEngine(rs::IEffect& e);
template <class T>
auto MakeCallDraw() { return rs::util::CallDraw<Engine>(&CnvToEngine); }
