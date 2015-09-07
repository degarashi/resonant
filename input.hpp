#pragma once
#ifdef WIN32
	#include <intrin.h>
#endif
#include "spinner/resmgr.hpp"
#include "input_dep_sdl.hpp"
#include <unordered_set>

namespace rs {
	//! アナログ入力値のデッドゾーンと倍率調整
	struct DZone {
		float ratio,
			deadzone;

		DZone(const DZone& dz) = default;
		DZone(float r=1.f, float dz=0);
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
		const static std::string		s_name;
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
		const static std::string		s_name;

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
			void setDeadZone(int num, float r, float dz) override;

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

			InputType getType() const override;
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
			InputType getType() const override;
			bool scan() override;
			WPtr getPointer() const override;

			void addListener(IRecvPointer* r) override;
			void remListener(IRecvPointer* r) override;

			static HLInput OpenTouchpad(int num);
			const std::string& name() const override;
	};

	struct InputFlag {
		enum E {
			Button,
			ButtonFlip,
			Axis,
			AxisNegative,
			AxisPositive,
			Hat,
			HatX,
			HatY,
			_Num
		};
	};
	namespace detail {
		class Action {
			public:
				// 値の取得
				using FGet = int (IInput::*)(int) const;
				// 値の改変
				using FManip = int (*)(int);
				struct Funcs {
					FGet	getter;
					FManip	manipulator;
				};
			private:
				const static Funcs cs_funcs[InputFlag::_Num];

				//! 入力機器からの値取得方法を定義
				struct Link {
					HLInput	hlInput;
					InputFlag::E	inF;
					int		num;

					bool operator == (const Link& l) const;
					int getValue() const;
				};
				using LinkV = std::vector<Link>;
				LinkV		_link;

				// ボタンを押している間は
				// Pressed=1, Neutral=0, Released=-1, Pressing= n>=1
				int			_state,
							_value;
				void _advanceState(int val);
			public:
				Action();
				Action(Action&& a) noexcept;
				void update();
				bool isKeyPressed() const;
				bool isKeyReleased() const;
				bool isKeyPressing() const;
				void addLink(HInput hI, InputFlag::E inF, int num);
				void remLink(HInput hI, InputFlag::E inF, int num);
				int getState() const;
				int getValue() const;
		};
		class ActMgr : public spn::ResMgrN<Action, ActMgr> {
			public:
				const std::string& getResourceName(spn::SHandle sh) const override;
		};
	}
	//! Actionとキーの割付とステート管理用
	/*! ボタンステートはAction毎に用意 */
	class InputMgr : protected InputMgrBase {
		private:
			using ActSet = std::unordered_set<HLAct>;
			detail::ActMgr	_act;
			ActSet			_aset;
			template <class CHK>
			bool _checkKeyValue(CHK chk, HAct hAct) const;
			template <std::size_t... N, class Tuple>
			static auto _getKeyValueSimplifiedMulti(std::index_sequence<N...>, const Tuple& t) {
				return std::make_tuple(getKeyValueSimplified(std::get<N>(t))...);
			}
			template <std::size_t... N>
			static void _linkButtonAsAxisMulti(std::index_sequence<N...>, HInput) {}
			template <std::size_t... N, class T, class... Tuple>
			static void _linkButtonAsAxisMulti(std::index_sequence<N...> seq, HInput hI, const T& t, const Tuple&... ts) {
				linkButtonAsAxis(hI, std::get<N>(t)...);
				_linkButtonAsAxisMulti(seq, hI, ts...);
			}
		public:
			InputMgr();
			~InputMgr();

			HLInput addInput(UPInput&& u);
			HLAct makeAction(const std::string& name);
			void addAction(HAct hAct);
			// 更新リストから除くだけで削除はしない
			void remAction(HAct hAct);
			static void linkButtonAsAxis(HInput hI, HAct hAct, int num_negative, int num_positive);
			template <class T, class... Tuple>
			static void linkButtonAsAxisMulti(HInput hI, const T& t, const Tuple&... ts) {
				using IntS = std::make_index_sequence<std::tuple_size<T>::value>;
				_linkButtonAsAxisMulti(IntS(), hI, t, ts...);
			}
			//! getValueの結果を使いやすいように加工(-1〜1)して返す
			/*! \retval 1	getValueの値がInputRangeHalf以上
						-1	getValueの値が-InputRangeHalf以下
						0	どちらでもない時 */
			static int getKeyValueSimplified(HAct hAct);
			//! 複数のハンドルを受け取りsimplifiedの結果をstd::tupleで返す
			template <class... H>
			static auto getKeyValueSimplifiedMulti(H... h) {
				using IntS = std::make_index_sequence<sizeof...(H)>;
				return _getKeyValueSimplifiedMulti(IntS(), std::forward_as_tuple(h...));
			}
			// キーマッピングのファイル入出力
			// 入力キャプチャコールバック
			using CaptureCB = bool (HInput, int);
			void addCaptureCB(CaptureCB cb);
			// 次回の更新時に処理
			void update();
	};
}
#include "luaimport.hpp"
DEF_LUAIMPORT(InputMgr)
DEF_LUAIMPORT(detail::Action)
