#pragma once
#include <memory>

namespace spn {
	template <int N, bool A>
	struct VecT;
	using Vec2 = VecT<2,false>;
}
namespace rs {
	namespace util {
		struct ColorA;
	}
	class LCValue;
	template <class T>
	class LValue;
	class LV_Stack;
	using LValueS = LValue<LV_Stack>;
	class LV_Global;
	using LValueG = LValue<LV_Global>;
}
namespace tweak {
	using Vec2 = spn::Vec2;
	using LValueS = rs::LValueS;
	using LValueG = rs::LValueG;
	using LCValue = rs::LCValue;
	//! 要素ごとの基本色
	struct Color {
		enum type {
			Linear,
			Log,
			Dir2D,
			Dir3D,
			Node,
			Cursor,
			_Num
		} c;
		const rs::util::ColorA& get() const;
	};
	class Drawer;
	//! Tweakツリーインタフェース
	struct IBase {
		virtual void set(const rs::LValueS& v, bool bStep);
		virtual void setInitial(const rs::LValueS& v);
		virtual void increment(float inc, int index);
		virtual int draw[[noreturn]](const spn::Vec2& offset,  const spn::Vec2& unit, Drawer& d) const;
		virtual rs::LCValue get() const;
	};
	class Value;
	using Value_UP = std::unique_ptr<Value>;
}
