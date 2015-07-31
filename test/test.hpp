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
#include "../glx.hpp"

namespace vertex {
	//! キューブ描画用頂点
	struct cube {
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
	struct cube {};
	struct sprite {};
}
DefineVDecl(::vdecl::cube)
DefineVDecl(::vdecl::sprite)

class Engine;
class Sc_Base;
// ゲーム通しての(MainThread, DrawThread含めた)グローバル変数
// リソースハンドルはメインスレッドで参照する -> メインスレッドハンドラに投げる
struct SharedValue {
	Engine*		pEngine;
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
				actNumber[5];
	rs::HLCam	hlCam;
};
#define sharedv (::rs::GameloopHelper<Engine, SharedValue, Sc_Base>::SharedValueC::_ref())
using GlxId = rs::GLEffect::GlxId;
extern const rs::GMessageId MSG_StateName;
rs::HLDObj MakeFBClear(rs::Priority priority);
extern const rs::IdValue T_Rect;

class Engine;
struct CnvToEngine {
	Engine& operator()(rs::GLEffect&) const;
};
