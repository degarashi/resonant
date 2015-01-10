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

class Cube : public spn::Pose3D {
	rs::HLVb	_hlVb;
	rs::HLTex	_hlTex;

	public:
		Cube(float s, rs::HTex hTex);
		void draw(rs::GLEffect& glx);
};
//! キューブObj(Update)
class CubeObj : public rs::ObjectT<CubeObj>, public spn::CheckAlign<16, CubeObj> {
	private:
		//! キューブObj(Draw)
		class CubeDraw : public rs::DrawableObjT<CubeDraw> {
			private:
				Cube&	_cube;
				GLint	_techId,
						_passId;
			public:
				CubeDraw(Cube& cube, GLint techId, GLint passId);
				void onDraw() const override;
		};

		// 本当はCubeTech等はグローバルに定義
		GLint		_techId,
					_passId;
		Cube		_cube;
		rs::HLDObj	_hlDraw;
		class MySt : public StateT<MySt> {
			public:
				void onUpdate(CubeObj& self);
		};
	public:
		CubeObj(rs::HTex hTex, GLint techId, GLint passId);
		~CubeObj();
		void onConnected(rs::HGroup hGroup) override;
		void onDisconnected(rs::HGroup hGroup) override;
};
extern const rs::GMessageId MSG_Visible;
class InfoShow : public rs::ObjectT<InfoShow> {
	private:
		bool			_bShow;
		class InfoDraw : public rs::DrawableObjT<InfoDraw> {
			private:
				const InfoShow&		_info;
				mutable rs::HLText	_hlText;
			public:
				InfoDraw(const InfoShow& is);
				void onDraw() const override;
		};

		GLint			_techId,
						_passId;
		std::u32string	_infotext;
		rs::CCoreID		_charId;

		rs::HLDObj		_hlDraw;
		class MySt : public StateT<MySt> {
			public:
				void onUpdate(InfoShow& self) override;
				rs::LCValue recvMsg(InfoShow& self, rs::GMessageId msg, const rs::LCValue& arg) override;
		};
	public:
		InfoShow(GLint techId, GLint passId);
		void onConnected(rs::HGroup hGroup) override;
		void onDisconnected(rs::HGroup hGroup) override;
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
	class MySt : public StateT<MySt> {
		public:
			void onEnter(TScene& self, rs::ObjTypeId prevId) override;
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
		void onConnected(rs::HGroup hGroup) override;
		void onDisconnected(rs::HGroup hGroup) override;
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

