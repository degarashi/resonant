#pragma once
#include "test/tweak/common.hpp"
#include "test/tweak/fvec.hpp"
#include "test/tweak/stext.hpp"
#include "font.hpp"

namespace tweak {
	using Vec3 = spn::Vec3;
	using DegF = spn::DegF;
	//! 調整可能な定数値
	class Value : public IBase {
		protected:
			const rs::CCoreID	_cid;
		public:
			Value(rs::CCoreID cid);
			virtual Value_UP clone() const = 0;
			virtual std::ostream& write(std::ostream& s) const = 0;
	};
	//! Valueクラスのcloneメソッドをテンプレートにて定義
	//! \sa Value
	template <class T, int N>
	class ValueT : public Value {
		private:
			mutable rs::HLText	_text[N];
		protected:
			mutable bool		_bRefl[N];
			virtual rs::HLText _getText(rs::CCoreID cid, int t) const = 0;
		public:
			struct Enum { enum {
				Value,
				Step,
				_Num
			}; };
			constexpr static int Dim = N;
			ValueT(const rs::CCoreID cid):
				Value(cid)
			{
				for(auto& b : _bRefl)
					b = true;
			}
			rs::HText getText(const int t) const {
				if(_bRefl[t]) {
					_bRefl[t] = false;
					_text[t] = _getText(_cid, t);
				}
				return _text[t];
			}
			virtual Value_UP clone() const override {
				return std::make_unique<T>(static_cast<const T&>(*this));
			}
	};
	//! 線形の増減値による定数調整
	class LinearValue : public ValueT<LinearValue, 2> {
		private:
			using base_t = ValueT<LinearValue, 2>;
			struct Text { enum {
				Name,
				_Num
			}; };
			FVec4		_data[Enum::_Num];
			StaticText	_stextL;
			rs::HLText _getText(rs::CCoreID cid, int t) const override;
		public:
			const static std::string Name;
			LinearValue(rs::CCoreID cid, int n);
			void loadDefine(const LValueS& tbl) override;
			void loadValue(const LValueS& v) override;
			void increment(float inc, int index) override;
			spn::SizeF draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
	//! 級数的な定数調整
	class ExpValue : public ValueT<ExpValue, 3> {
		public:
			struct Enum { enum {
				Value,
				Step,
				Base,
				_Num
			}; };
		private:
			using base_t = ValueT<ExpValue, 3>;
			struct Text { enum {
				Name,
				Base,
				_Num
			}; };
			FVec4		_data[Enum::_Num];
			StaticText	_stextE;
			rs::HLText _getText(rs::CCoreID cid, int t) const override;
			FVec4 _getRaw() const;
		public:
			const static std::string Name;
			ExpValue(rs::CCoreID cid, int n);
			void loadDefine(const LValueS& tbl) override;
			void loadValue(const LValueS& v) override;
			void increment(float inc, int index) override;
			spn::SizeF draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
	//! 2D方向ベクトル定数
	class Dir2D : public ValueT<Dir2D, 2> {
		private:
			DegF		_angle,
						_step;
			struct Text { enum {
				Name,
				_Num
			}; };
			StaticText	_stextD2;
			Vec2 _getRaw() const;
			rs::HLText _getText(rs::CCoreID cid, int t) const override;
		public:
			const static std::string Name;
			Dir2D(rs::CCoreID cid);
			void loadDefine(const LValueS& tbl) override;
			void loadValue(const LValueS& v) override;
			void increment(float inc, int index) override;
			spn::SizeF draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
	//! 3D方向ベクトル(Yaw, Pitch)
	class Dir3D : public ValueT<Dir3D, 2> {
		private:
			DegF		_angle[2],
						_step[2];
			struct Text { enum {
				Name,
				_Num
			}; };
			StaticText	_stextD3;
			Vec3 _getRaw() const;
			rs::HLText _getText(rs::CCoreID cid, int t) const override;
		public:
			const static std::string Name;
			Dir3D(rs::CCoreID cid);
			void loadDefine(const LValueS& tbl) override;
			void loadValue(const LValueS& v) override;
			void increment(float inc, int index) override;
			spn::SizeF draw(const Vec2& offset, const Vec2& unit, Drawer& d) const override;
			spn::SizeF drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const override;
			rs::LCValue get() const override;
			std::ostream& write(std::ostream& s) const override;
	};
}
