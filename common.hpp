#pragma once
#include "spinner/vector.hpp"
#include "spinner/resmgr.hpp"
#include <set>
#include "handle.hpp"

namespace rs {
	constexpr int 	InputRange = 1024,
					InputRangeHalf = InputRange/2;

	struct TPos2D {
		bool		bTouch;
		// 画面に対する0〜1の比率で記録する
		spn::Vec2	absPos,
					relPos;
		float		pressure;

		static struct tagClean {} Clean;
		TPos2D() = default;
		TPos2D(tagClean): bTouch(false), absPos{0}, relPos{0}, pressure(0) {}
		void setNewAbs(const spn::Vec2& p) {
			relPos.x = p.x - absPos.x;
			relPos.y = p.y - absPos.y;
			absPos = p;
		}
		void setNewRel(const spn::Vec2& p) {
			absPos += p;
			relPos = p;
		}
	};

	#define mgr_pointer ::rs::PointerMgr::_ref()
	class PointerMgr : public spn::ResMgrA<TPos2D, PointerMgr> {};
	using PtrNS = spn::noseq_list<HLPtr>;

	struct IRecvPointer {
		virtual void newPointer(WPtr wptr) = 0;
		virtual ~IRecvPointer() {}
	};
	struct RecvPtrGroup : IRecvPointer {
		using LSet = std::set<IRecvPointer*>;
		LSet	listener;

		void newPointer(WPtr wptr) override;
	};
	struct RecvPtrDummy : IRecvPointer {
		void newPointer(WPtr wptr) override {}
	};
	enum class InputType {
		Keyboard,
		Mouse,
		Joypad,
		Touchpad
	};
	enum class MouseMode {
		Absolute,
		Clipping,
		Relative
	};

	//! 入力共通インタフェース
	class IInput {
		bool _bActive;
		public:
			virtual ~IInput() {}
			void activate(bool bActive);
			bool isActive() const;

			virtual InputType getType() const = 0;
			// エラー時はfalseを返す
			virtual bool scan() = 0;
			virtual const std::string& name() const = 0;
			// ---- relative input querying ----
			virtual int getButton(int num) const { return 0;}
			virtual int getAxis(int num) const { return 0; }
			virtual int getHat(int num) const { return -1; }
			virtual int numButtons() const { return 0; }
			virtual int numAxes() const { return 0; }
			virtual int numHats() const { return 0; }
			virtual void setDeadZone(int num, float r, float dz) {}

			virtual void setMouseMode(MouseMode mode) {}
			virtual MouseMode getMouseMode() const { return MouseMode::Absolute; }
			// ---- absolute input querying ----
			virtual void addListener(IRecvPointer* r) {}
			virtual void remListener(IRecvPointer* r) {}
			// 1つ以上の座標が検出されたら何れかを返す
			virtual WPtr getPointer() const { return WPtr(); }
	};

	#define mgr_inputb (::rs::InputMgrBase::_ref())
	#define mgr_input reinterpret_cast<::rs::InputMgr&>(::rs::InputMgrBase::_ref())
	class InputMgrBase : public spn::ResMgrA<UPInput, InputMgrBase> {};
}
