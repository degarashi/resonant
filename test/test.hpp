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
#include "../camera.hpp"
#include "../scene.hpp"
#include "../input.hpp"
#include "../sound.hpp"
#include "../font.hpp"
#include "../vertex.hpp"
#include "../sys_uniform.hpp"
#include "../glx.hpp"
#include "../differential.hpp"

namespace vertex {
	//! キューブ描画用頂点
	struct cube {
		spn::Vec3	pos;
		spn::Vec2	tex;
		spn::Vec3	normal;
	};
}
namespace drawtag {
	struct cube {};
}
DefineVDecl(::drawtag::cube)

class Engine;
// ゲーム通しての(MainThread, DrawThread含めた)グローバル変数
// リソースハンドルはメインスレッドで参照する -> メインスレッドハンドラに投げる
struct SharedValue {
	Engine*		pEngine;
	rs::HLAct	actQuit,
				actLeft,
				actRight,
				actUp,
				actDown,
				actMoveX,
				actMoveY,
				actPress,
				actReset,
				actPlay,
				actStop;
	rs::HLCam	hlCam;
};
#define sharedv (::rs::GameloopHelper<Engine, SharedValue, TScene>::SharedValueC::_ref())

using GlxId = rs::GLEffect::GlxId;

class Cube : public spn::Pose3D {
	private:
		rs::HLVb	_hlVb;
		rs::HLTex	_hlTex;
		const static rs::IdValue	U_litdir;
	public:
		Cube(float s, rs::HTex hTex);
		void exportDrawTag(rs::DrawTag& d) const;
		void draw(Engine& e);
};

//! キューブObj(Update)
class CubeObj : public rs::DrawableObjT<CubeObj, 0x0000>,
				public spn::CheckAlign<16, CubeObj>,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::IdValue		_tpId;
		mutable Cube	_cube;
		class MySt : public StateT<MySt> {
			public:
				void onUpdate(CubeObj& self) override;
				void onConnected(CubeObj& self, rs::HGroup hGroup) override;
				void onDisconnected(CubeObj& self, rs::HGroup hGroup) override;
				void onDraw(const CubeObj& self) const override;
		};
		void initState() override;
	public:
		CubeObj(rs::HTex hTex, rs::IdValue tpId);
		~CubeObj();
};
extern const rs::GMessageId MSG_Visible;
class InfoShow : public rs::DrawableObjT<InfoShow, 0x1000>,
				public spn::EnableFromThis<rs::HDObj>
{
	private:
		rs::IdValue			_tpId;
		std::u32string		_infotext;
		rs::CCoreID			_charId;
		mutable rs::HLText	_hlText;
		rs::diff::Effect	_count;

		struct MySt;
		void initState() override;
	public:
		InfoShow(rs::IdValue tpId);
};

class TScene2 : public rs::Scene<TScene2> {
	private:
		class MySt : public StateT<MySt> {
			public:
				void onUpdate(TScene2& self) override;
		};
		void initState() override;
	public:
		TScene2();
		~TScene2();
};

extern const rs::GMessageId MSG_GetStatus;
class TScene : public rs::Scene<TScene> {
	const static rs::IdValue	T_Info,
								T_Cube;
	rs::HLAb	_hlAb;
	rs::HLSg	_hlSg;
	rs::HDObj	_hInfo;
	rs::HDObj	_hCube;
	bool		_bPress;
	spn::DegF	_yaw, _pitch, _roll;

	struct St_Init;
	struct St_Idle;
	struct St_Play;
	void initState() override;

	public:
		TScene();
		~TScene();
};

class Engine : public rs::SystemUniform3D,
			public rs::SystemUniform,
			public rs::GLEffect
{
	public:
		using rs::GLEffect::GLEffect;
		void drawIndexed(GLenum mode, GLsizei count, GLuint offsetElem=0) override;
		void draw(GLenum mode, GLint first, GLsizei count) override;
};

