#pragma once
#include "test/tweak/common.hpp"
#include "fvec.hpp"
#include "../../font.hpp"

namespace tweak {
	using Vec3 = spn::Vec3;
	using DegF = spn::DegF;
	struct VStep {
		enum type {
			Value,
			Step,
			_Num
		} v;
	};
	struct SText {
		enum type {
			Type,
			_Num
		} v;
	};
	//! 調整可能な定数値
	class Value : public IBase {
		protected:
			constexpr static size_t NText = VStep::_Num + SText::_Num;
			const rs::CCoreID	_cid;
			mutable bool		_bRefl[VStep::_Num];
		private:
			mutable rs::HLText	_text[NText];
			virtual rs::HLText _getText(VStep::type t) const = 0;
			virtual rs::HLText _getText(SText::type t) const = 0;
		public:
			Value(rs::CCoreID cid);
			rs::HText getText(VStep::type t) const;
			virtual rs::HText getText(SText::type t) const = 0;
			virtual Value_UP clone() const = 0;
			virtual std::ostream& write(std::ostream& s) const = 0;
	};
	//! Valueクラスのcloneメソッドをテンプレートにて定義
	//! \sa Value
	template <class T>
	class ValueT : public Value {
		protected:
			mutable rs::HLText _stext[SText::_Num];
		public:
			using Value::Value;
			using Value::getText;
			rs::HText getText(SText::type t) const override {
				if(!_stext[t])
					_stext[t] = _getText(t);
				return _stext[t];
			}
			virtual Value_UP clone() const override {
				return std::make_unique<T>(static_cast<const T&>(*this));
			}
	};
	//! 線形の増減値による定数調整
	class LinearValue : public ValueT<LinearValue> {
		private:
			rs::HLText _getText(VStep::type t) const override;
			rs::HLText _getText(SText::type t) const override;
		protected:
			using base_t = ValueT<LinearValue>;
			FVec4		_data[VStep::_Num];
		public:
			LinearValue(rs::CCoreID cid, int n);
			void set(const LValueS& v, bool bStep) override;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
	//! 級数的な定数調整
	class LogValue : public LinearValue {
		private:
			rs::HLText _getText(SText::type t) const override;
		public:
			using LinearValue::LinearValue;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			Value_UP clone() const override;
	};
	//! 2D方向ベクトル定数
	class Dir2D : public ValueT<Dir2D> {
		private:
			DegF		_angle,
						_step;
			Vec2 _getRaw() const;
			rs::HLText _getText(VStep::type t) const override;
			rs::HLText _getText(SText::type t) const override;
		public:
			Dir2D(rs::CCoreID cid);
			void set(const LValueS& v, bool bStep) override;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
	//! 3D方向ベクトル(Yaw, Pitch)
	class Dir3D : public ValueT<Dir3D> {
		private:
			DegF		_angle[2],
						_step[2];
			Vec3 _getRaw() const;
			rs::HLText _getText(VStep::type t) const override;
			rs::HLText _getText(SText::type t) const override;
		public:
			Dir3D(rs::CCoreID cid);
			void set(const LValueS& v, bool bStep) override;
			void increment(float inc, int index) override;
			int draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
}
