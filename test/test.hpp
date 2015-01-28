#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif

#include "spinner/pose.hpp"
#include "spinner/vector.hpp"
#include "gameloop.hpp"
#include "updater.hpp"
#include "scene.hpp"
#include "glx.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "font.hpp"
#include "prochelper.hpp"
#include "vertex.hpp"
#include "glx_id.hpp"

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
DefineDrawParam(::drawtag::cube)

// ゲーム通しての(MainThread, DrawThread含めた)グローバル変数
// リソースハンドルはメインスレッドで参照する -> メインスレッドハンドラに投げる
struct SharedValue {
	rs::GLEffect*	pFx;
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
#define shared	(::SharedValueC::_ref())
constexpr int Id_SharedValueC = 0xf0000001;
class SharedValueC : public spn::Singleton<SharedValueC>, public rs::GLSharedData<SharedValue, Id_SharedValueC> {};

class MyDraw : public rs::DrawProc {
	public:
		bool runU(uint64_t accum, bool bSkip) override;
};

struct myTag {};
using MyId = rs::IdMgr_Glx<myTag>;
extern MyId g_myId;

class Cube : public spn::Pose3D {
	private:
		rs::HLVb	_hlVb;
		rs::HLTex	_hlTex;
		const static rs::IdValue	U_diffuse,
									U_trans,
									U_litdir;
	public:
		Cube(float s, rs::HTex hTex);
		void draw(rs::GLEffect& glx);
};

//! キューブObj(Update)
class CubeObj : public rs::DrawableObjT<CubeObj, 0x0000>, public spn::CheckAlign<16, CubeObj>, public spn::EnableFromThis<rs::HDObj> {
	private:
		// 本当はCubeTech等はグローバルに定義
		GLint			_techId,
						_passId;
		mutable Cube	_cube;
		class MySt : public StateT<MySt> {
			public:
				void onUpdate(CubeObj& self) override;
				void onConnected(CubeObj& self, rs::HGroup hGroup) override;
				void onDisconnected(CubeObj& self, rs::HGroup hGroup) override;
				void onDraw(const CubeObj& self) const override;
		};
	public:
		CubeObj(rs::HTex hTex, GLint techId, GLint passId);
		~CubeObj();
};
extern const rs::GMessageId MSG_Visible;
class InfoShow : public rs::DrawableObjT<InfoShow, 0x1000>, public spn::EnableFromThis<rs::HDObj> {
	private:
		GLint				_techId,
							_passId;
		std::u32string		_infotext;
		rs::CCoreID			_charId;
		mutable rs::HLText	_hlText;

		class MySt : public StateT<MySt> {
			public:
				rs::LCValue recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) override;
				void onConnected(InfoShow& self, rs::HGroup hGroup) override;
				void onDisconnected(InfoShow& self, rs::HGroup hGroup) override;
				void onDraw(const InfoShow& self) const override;
		};
	public:
		InfoShow(GLint techId, GLint passId);
};

class TScene2 : public rs::Scene<TScene2> {
	class MySt : public StateT<MySt> {
		public:
			void onUpdate(TScene2& self) override;
	};
	public:
		TScene2();
		~TScene2();
};

extern const rs::GMessageId MSG_GetStatus;
class TScene : public rs::Scene<TScene> {
	rs::HLAb	_hlAb;
	rs::HLSg	_hlSg;
	rs::HDObj	_hInfo;
	rs::HDObj	_hCube;
	class MySt : public StateT<MySt> {
		public:
			void onEnter(TScene& self, rs::ObjTypeId prevId) override;
			void onExit(TScene& self, rs::ObjTypeId nextId) override;
			void onUpdate(TScene& self) override;
			void onDown(TScene& self, rs::ObjTypeId prevId, const rs::LCValue& arg) override;
			void onPause(TScene& self) override;
			void onResume(TScene& self) override;
			void onStop(TScene& self) override;
			void onReStart(TScene& self) override;
			rs::LCValue recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) override;
			static void CheckQuit();
	};
	class MySt_Play : public StateT<MySt_Play> {
		public:
			void onEnter(TScene& self, rs::ObjTypeId prevId) override;
			void onUpdate(TScene& self) override;
			rs::LCValue recvMsg(TScene& self, rs::GMessageId msg, const rs::LCValue& arg) override;
	};
	public:
		TScene();
		~TScene();
};

class MyMain : public rs::MainProc {
	private:
		SharedValueC	_svalue;
		bool			_bPress;
		spn::DegF		_yaw, _pitch, _roll;

		void	_initInput();
		void	_initEffect();
		void	_initCam();
	public:
		MyMain(const rs::SPWindow& sp);
		bool userRunU() override;
};

