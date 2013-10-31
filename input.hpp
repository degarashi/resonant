#pragma once
#include "spinner/resmgr.hpp"
#include "input_dep_sdl.hpp"
#include <unordered_set>

namespace rs {
	struct DZone {
		int deadzone;

		DZone(const DZone& dz) = default;
		DZone(int dz=0);
		int filter(int val) const;
	};
	using DZoneL = std::vector<DZone>;
	enum VKey {
		VKEY_0, VKEY_1, // ...
		NUM_VKEY
	};
	// Class <-> Dep の間は数値を正規化 (-InputRange +InputRange)
	/*! Depend要件:
		protected:
			(キーボードに関しては最大キー数は関係なく、キーコードで問い合わせる)
			int dep_getButton(int) const;	<= 引数はキーコード
			bool dep_scan();
			template <class> static HLInput OpenKeyboard();
		public:
			static int NumKeyboards(); */
	class Keyboard : public KeyboardDep, public IInput {
		static std::string		s_name;
		public:
			using KeyboardDep::KeyboardDep;
			InputType getType() const override;
			int getButton(int num) const override;
			int numButtons() const override;
			bool scan() override;
			const std::string& name() const override;

			static HLInput OpenKeyboard();
	};

	/*! Depend要件:
		protected:
			template <T> static T* OpenMouse(int num);
			int dep_numButtons() const;
			int dep_numAxes() const;
			int dep_getButton(int) const;
			int dep_getAxis(int) const;
			bool dep_scan(TPos2D&);
		public:
			static void Update();
			static int NumMouse();
			static std::string GetMouseName(int);
			void setWindowClip(bool); */
	class Mouse : public MouseDep, public IInput {
		DZoneL		_axisDZ = DZoneL(MouseDep::dep_numAxes());
		HLPtr		_hlPtr = mgr_pointer.acquire(TPos2D(TPos2D::Clean));

		public:
			using MouseDep::MouseDep;
			InputType getType() const override;
			bool scan() override;
			const std::string& name() const override;
			int getButton(int num) const override;
			int getAxis(int num) const override;
			int numButtons() const override;
			int numAxes() const override;
			void setMouseMode(MouseMode mode) override;
			MouseMode getMouseMode() const override;
			WPtr getPointer() const override;

			static HLInput OpenMouse(int num);
	};

	/*! Depend要件:
		protected:
			static void Update();
			template <class> static HLInput OpenJoypad(int);
			int dep_numButtons() const;
			int dep_numAxes() const;
			int dep_numHats() const;
			int dep_getButton(int) const;
			int dep_getAxis(int) const;
			int dep_getHat(int) const;
			bool dep_scan();
		public:
			static int NumJoypad();
			static std::string GetJoypadName(int);
			const std::string& name() const; */
	class Joypad : public JoypadDep, public IInput {
		DZoneL		_axisDZ = DZoneL(JoypadDep::dep_numAxes());

		public:
			using JoypadDep::JoypadDep;

			InputType getType() const;
			bool scan() override;
			int getButton(int num) const override;
			int getAxis(int num) const override;
			int getHat(int num) const override;
			int numButtons() const override;
			int numAxes() const override;
			int numHats() const override;

			static HLInput OpenJoypad(int num);
			const std::string& name() const override;
	};
	// タッチパネル関係はイベントでしか取得できない？
	/*! Depend要件:
		protected:
			template <class> static HLInput OpenTouchpad(int);
			bool dep_scan(IRecvPointer*);
		public:
			static void Update();
			static int NumTouchpad();
			static std::string GetTouchpadName(int);
			const std::string& name() const; */
	class Touchpad : public TouchDep, public IInput {
		RecvPtrGroup	_group;
		public:
			using TouchDep::TouchDep;
			InputType getType() const;
			bool scan() override;
			WPtr getPointer() const override;

			void addListener(IRecvPointer* r) override;
			void remListener(IRecvPointer* r) override;

			static HLInput OpenTouchpad(int num);
			const std::string& name() const override;
	};

	//! 入力機器からの値取得方法を定義
	class InF {
		// 値の取得
		using FGet = int (IInput::*)(int) const;
		// 値の改変
		using FManip = int (*)(int);

		HLInput	_hlInput;
		int		_num;
		FGet	_fGet;
		FManip	_fManip;

		InF(HInput hI, int num, FGet fGet, FManip fManip);

		public:
			InF(const InF& f) = default;
			int getValue() const;
			bool operator == (const InF& f) const;

			static InF AsButton(HInput hI, int num);		// しきい値指定バージョンも用意
			static InF AsAxis(HInput hI, int num);
			static InF AsAxisNegative(HInput hI, int num);
			static InF AsAxisPositive(HInput hI, int num);
			static InF AsHat(HInput hI, int num);
			static InF AsHatX(HInput hI, int num);
			static InF AsHatY(HInput hI, int num);
	};

	//! Actionとキーの割付とステート管理用
	/*! ボタンステートはAction毎に用意 */
	class InputMgr : protected InputMgrBase {
		private:
			using Link = std::vector<InF>;
			class Action {
				Link		_link;
				// ボタンを押している間は
				// Pressed=1, Neutral=0, Released=-1, Pressing= n>=1
				int			_state,
							_value;
				void _advanceState(int val);
				public:
					Action();
					Action(Action&& a) noexcept;
					void update();
					void addLink(const InF& inF);
					void remLink(const InF& inF);
					int getState() const;
					int getValue() const;
			};
			class ActMgr : public spn::ResMgrN<Action, ActMgr> {} _act;
			PointerMgr _ptr;
		public:
			using HLAct = ActMgr::AnotherLHandle<Action>;
			using HAct = ActMgr::AnotherSHandle<Action>;
		private:
			using ActSet = std::unordered_set<HLAct>;
			ActSet	_aset;
			template <class CHK>
			bool _checkKeyValue(CHK chk, HAct hAct) const {
				return chk(hAct.ref().getState());
			}
		public:
			InputMgr();
			~InputMgr();
			bool isKeyPressed(HAct hAct) const;
			bool isKeyReleased(HAct hAct) const;
			bool isKeyPressing(HAct hAct) const;

			HLInput addInput(UPInput&& u);
			HLAct addAction(const std::string& name);
			void addAction(HAct hAct);
			// 更新リストから除くだけで削除はしない
			void remAction(HAct hAct);
			void link(HAct hAct, const InF& inF);
			void unlink(HAct hAct, const InF& inF);	// linkを個別に解除

			// キーマッピングのファイル入出力
			// 入力キャプチャコールバック
			using CaptureCB = bool (HInput, int);
			void addCaptureCB(CaptureCB cb);
			// 次回の更新時に処理
			void update();
	};
}
