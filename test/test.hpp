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
	//! プリミティブ描画用頂点
	struct prim {
		spn::Vec3	pos;
		spn::Vec2	tex;
		spn::Vec3	normal;
	};
	//! スプライト描画用頂点
	struct sprite {
		spn::Vec3	pos;
		spn::Vec2	tex;
	};
}
namespace vdecl {
	struct prim {};
	struct sprite {};
}
DefineVDecl(::vdecl::prim)
DefineVDecl(::vdecl::sprite)

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

class Engine;
Engine& CnvToEngine(rs::IEffect& e);
template <class T>
auto MakeCallDraw() { return rs::util::CallDraw<Engine>(&CnvToEngine); }
