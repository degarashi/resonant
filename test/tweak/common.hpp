#pragma once
#include <memory>

namespace spn {
	template <int N, bool A>
	struct VecT;
	using Vec2 = VecT<2,false>;
	template <class T>
	struct _Size;
	using SizeF = _Size<float>;
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
	struct STextPack;
	//! 要素ごとの基本色
	struct Color {
		enum type {
			Linear,
			Exp,
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
		virtual void loadDefine(const rs::LValueS& tbl);
		virtual void loadValue(const rs::LValueS& v);
		virtual void increment(float inc, int index);
		//! Current = Initial
		virtual void reset();
		//! Initial = Current
		virtual void setAsInitial();
		virtual spn::SizeF draw[[noreturn]](const spn::Vec2& offset,  const spn::Vec2& unit, Drawer& d) const;
		virtual spn::SizeF drawInfo[[noreturn]](const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const = 0;
		virtual rs::LCValue get() const;
	};
	class Value;
	using Value_UP = std::unique_ptr<Value>;
	namespace entry {
		extern const std::string
			Variable,
			Value,
			Step,
			Base,
			Manip,
			Apply;
	}
}
