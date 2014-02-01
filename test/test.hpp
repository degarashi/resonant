#ifdef WIN32
	#include <intrin.h>
	#include <windows.h>
#endif

#include "spinner/pose.hpp"
#include "spinner/vector.hpp"
#include "gameloop.hpp"
#include "updator.hpp"
#include "scene.hpp"
#include "glx.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "input.hpp"
#include "sound.hpp"
#include "font.hpp"

// MainThread と DrawThread 間のデータ置き場
// リソースハンドルはメインスレッド
struct Mth_DthData {
	rs::HLFx	hlFx;
	rs::HLInput	hlIk,
				hlIm;
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
	rs::SPWindow	spWin;
	rs::FPSCounter	fps;
};
#define shared (Mth_Dth::_ref())
class Mth_Dth : public spn::Singleton<Mth_Dth>, public rs::GLSharedData<Mth_DthData> {};

class MyDraw : public rs::IDrawProc {
	public:
		bool runU(uint64_t accum) override;
};

class Cube : public spn::Pose3D {
	rs::HLVb	_hlVb;
	rs::HLTex	_hlTex;
	GLint		_techID, _passID;

	public:
		Cube(float s, rs::HTex hTex);
		void draw(rs::GLEffect& glx);
};

//! キューブの描画
class CubeObj : public rs::ObjectT<CubeObj>, public spn::CheckAlign<16, CubeObj> {
	Cube _cube;
	class MySt : public StateT<MySt> {
		public:
			void onUpdate(CubeObj& self);
	};
	public:
		CubeObj(rs::HTex hTex);
		~CubeObj();
		void onDestroy() override;
};

extern spn::Optional<Cube>		g_cube;
// class TScene2 : public rs::Scene<TScene2> {
// 	class MySt : public StateT<MySt> {
// 		public:
// 			void onUpdate(TScene2& self) override;
// 	};
// 	public:
// 		TScene2();
// 		~TScene2();
// };

extern const rs::GMessageID MSG_CallFunc;
extern const rs::GMessageID MSG_GetStatus;
class TScene : public rs::Scene<TScene> {
	rs::HLAb	_hlAb;
	rs::HLSg	_hlSg;
	class MySt : public StateT<MySt> {
		public:
			void onEnter(TScene& self, rs::ObjTypeID prevID) override;
			void onUpdate(TScene& self) override;
			void onDown(TScene& self, rs::ObjTypeID prevID, const rs::Variant& arg) override;
			void onPause(TScene& self) override;
			void onResume(TScene& self) override;
			void onStop(TScene& self) override;
			void onReStart(TScene& self) override;
			rs::Variant recvMsg(TScene& self, rs::GMessageID msg, rs::Variant arg) override;
			static void CheckQuit();
	};
	class MySt_Play : public StateT<MySt_Play> {
		public:
			void onEnter(TScene& self, rs::ObjTypeID prevID) override;
			void onUpdate(TScene& self) override;
			rs::Variant recvMsg(TScene& self, rs::GMessageID msg, rs::Variant arg) override;
	};
	void _drawCube();
	public:
		TScene();
		~TScene();
		void onDestroy() override;
};

class MyMain : public rs::IMainProc {
	Mth_Dth 	_mth;
	spn::Size	_size;
	bool		_bPress;

	// ---- テスト描画用 ----
	rs::HLVb		_hlVb;
	rs::HLIb 		_hlIb;
	rs::HLTex 		_hlTex;
	std::u32string	_infotext;
	rs::CCoreID		_charID;
	rs::HLText		_hlText;
	GLint			_techID,
					_passView,
					_passText;

	void _initInput();
	void _initDraw();
	void _initCam();
	void _initEffect();

	public:
		MyMain(const rs::SPWindow& sp);
		bool runU() override;
		void onPause() override;
		void onResume() override;
		void onStop() override;
		void onReStart() override;
};
