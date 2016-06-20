#include "test/tweak/value.hpp"
#include "test/tweak/drawer.hpp"
#include "spinner/rectdiff.hpp"

#include "spinner/structure/angle.hpp"
namespace spn {
	template <class V, class T>
	constexpr T Angle<V, T>::OneRotationAng;
}
namespace tweak {
	// ---------------- Value ----------------
	Value::Value(rs::CCoreID cid):
		_cid(cid)
	{}
	namespace {
		rs::HLText MakeValueText(rs::CCoreID cid, const float* value, const int n) {
			Assert(Trap, n>0)
			// 個々の数値 -> string
			std::string buff = std::to_string(value[0]);
			for(int i=1 ; i<n ; i++) {
				buff += ", ";
				buff += std::to_string(value[i]);
			}
			return mgr_text.createText(cid, std::move(buff));
		}
		spn::SizeF DrawText(Drawer& d, const spn::Vec2& offset, const spn::Vec2& unit, rs::HText hText, Color t) {
			auto sz = hText->getSize();
			sz.height *= -1;
			// 背景の矩形 with Wireframe
			d.drawRectBoth({
				offset.x,
				offset.x + sz.width,
				offset.y,
				offset.y + sz.height
			}, t);
			d.drawText(offset, hText, t);
			return {sz.width, unit.y};
		}
		template <class T>
		spn::SizeF DrawInfo(const T* self, rs::HText hName,
				const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d)
		{
			const Color color{Color::Node};
			auto ofs = offset;
			const int Spacing = unit.x;
			int mx = 0;
			// "Type" Type
			mx = d.drawTexts(ofs, color,
							st.getText(STextPack::Type), Spacing,
							hName, 0);
			ofs.y += unit.y;
			// "Step" Step
			mx = std::max<int>(
					mx,
					d.drawTexts(ofs, color,
						st.getText(STextPack::Step), Spacing,
						self->getText(T::Enum::Step), 0)
				);
			ofs.y += unit.y;
			// "Current" CurrentValue
			mx = std::max<int>(
					mx,
					d.drawTexts(ofs, color,
						st.getText(STextPack::Current), Spacing,
						self->getText(T::Enum::Value), 0)
				);
			ofs.y += unit.y;
			return {mx, ofs.y-offset.y};
		}
	}
	// ---------------- LinearValue ----------------
	namespace {
		const std::string cs_linearStr[] = {
			"Linear"
		};
		void SetValue(FVec4& dst, const LValueS& v) {
			const auto np = dst.nValue;
			dst = FVec4::LoadFromLua(v);
			if(dst.nValue != np) {
				if(dst.nValue == 1) {
					// 数値をベクトルへ拡張
					dst.nValue = np;
					dst = FVec4(np, dst.value.x);
				} else {
					// エラー
					AssertF(Trap, "incompatible default value")
				}
			}
		}
	}
	LinearValue::LinearValue(rs::CCoreID cid, const int n):
		base_t(cid),
		_data{
			{n,0},
			{n,1}
		},
		_stextL(cid, countof(cs_linearStr), cs_linearStr)
	{
		static_assert(base_t::Dim == Enum::_Num, "");
	}
	void LinearValue::loadDefine(const LValueS& tbl) {
		const auto f = [this, &tbl](const auto idx, const auto name){
			SetValue(_data[idx], tbl[name]);
			_bRefl[idx] = true;
		};
		loadValue(tbl[entry::Value]);
		f(Enum::Step, entry::Step);
	}
	void LinearValue::loadValue(const LValueS& v) {
		SetValue(_data[Enum::Value], v);
		_bRefl[Enum::Value] = true;
	}
	void LinearValue::increment(const float inc, const int index) {
		if(index < _data[Enum::Value].nValue) {
			_data[Enum::Value].value.m[index] += _data[Enum::Step].value.m[index] * inc;
			_bRefl[Enum::Value] = true;
		}
	}
	rs::HLText LinearValue::_getText(rs::CCoreID cid, const int t) const {
		return MakeValueText(cid, _data[t].value.m, _data[t].nValue);
	}
	spn::SizeF LinearValue::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		return DrawText(d, offset, unit, getText(Enum::Value), {Color::Linear});
	}
	spn::SizeF LinearValue::drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const {
		return DrawInfo(this, _stextL.getText(Text::Name), offset, unit, st, d);
	}
	rs::LCValue LinearValue::get() const {
		return _data[Enum::Value].toLCValue();
	}
	std::ostream& LinearValue::write(std::ostream& s) const {
		return _data[Enum::Value].write(s);
	}
	const std::string LinearValue::Name("linear");
	
	// ---------------- ExpValue ----------------
	namespace {
		const std::string cs_expStr[] = {
			"Exponential",
			"Base"
		};
	}
	ExpValue::ExpValue(rs::CCoreID cid, const int n):
		base_t(cid),
		_data{
			{n,0},	// Value
			{n,1},	// Step
			{n,2},	// Base
		},
		_stextE(cid, countof(cs_expStr), cs_expStr)
	{
		static_assert(base_t::Dim == Enum::_Num, "");
	}
	void ExpValue::loadDefine(const LValueS& tbl) {
		const auto f = [this, &tbl](const auto idx, const auto name){
			SetValue(_data[idx], tbl[name]);
			_bRefl[idx] = true;
		};
		f(Enum::Step, entry::Step);
		f(Enum::Base, entry::Base);
		// Baseを基準に値を計算するのでBaseより後でセット
		loadValue(tbl[entry::Value]);
	}
	void ExpValue::loadValue(const LValueS& v) {
		const auto& base = _data[Enum::Base];
		auto& val = _data[Enum::Value];
		SetValue(val, v);
		_bRefl[Enum::Value] = true;
		const int nv = base.nValue;
		for(int i=0 ; i<nv ; i++)
			val.value.m[i] = std::log(val.value.m[i]) / std::log(base.value.m[i]);
	}
	void ExpValue::increment(const float inc, const int index) {
		if(index < _data[Enum::Value].nValue) {
			_data[Enum::Value].value.m[index] += _data[Enum::Step].value.m[index] * inc;
			_bRefl[Enum::Value] = true;
		}
	}
	spn::SizeF ExpValue::draw(const Vec2& offset, const Vec2& unit, Drawer& d) const {
		return DrawText(d, offset, unit, getText(Enum::Value), {Color::Exp});
	}
	spn::SizeF ExpValue::drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const {
		auto sz = DrawInfo(this, _stextE.getText(Text::Name), offset, unit, st, d);
		// Base
		int mx = d.drawTexts(
			{
				offset.x,
				offset.y + sz.height
			},
			{Color::Node},
			st.getText(STextPack::Base), unit.x,
			getText(Enum::Base), 0
		);
		return {std::max<int>(sz.width, mx), offset.y+sz.height+unit.y};
	}
	rs::HLText ExpValue::_getText(rs::CCoreID cid, const int t) const {
		FVec4 tmp;
		if(t == Enum::Value)
			tmp = _getRaw();
		else
			tmp = _data[t];
		return MakeValueText(cid, tmp.value.m, tmp.nValue);
	}
	rs::LCValue ExpValue::get() const {
		return _getRaw().toLCValue();
	}
	FVec4 ExpValue::_getRaw() const {
		const auto &v = _data[Enum::Value],
				&b = _data[Enum::Base];
		const int nv = v.nValue;
		FVec4 tmp;
		tmp.nValue = nv;
		for(int i=0 ; i<nv ; i++)
			tmp.value.m[i] = std::pow(b.value.m[i], v.value.m[i]);
		return tmp;
	}
	std::ostream& ExpValue::write(std::ostream& s) const {
		return _getRaw().write(s);
	}
	const std::string ExpValue::Name("exp");
	
	// ---------------- Dir2D ----------------
	namespace {
		const std::string cs_d2Str[] = {
			"Dir2D"
		};
	}
	Dir2D::Dir2D(rs::CCoreID cid):
		ValueT(cid),
		_angle(0),
		_step(1),
		_stextD2(cid, countof(cs_d2Str), cs_d2Str)
	{}
	void Dir2D::loadDefine(const LValueS& tbl) {
		loadValue(tbl[entry::Value]);
		// Step
		{
			const rs::LValueS v(tbl[entry::Step]);
			AssertP(Throw, v.type()==rs::LuaType::Number)
			_step.set(v.toNumber());
			_bRefl[Enum::Step] = true;
		}
	}
	void Dir2D::loadValue(const LValueS& v) {
		AssertP(Throw, v.type()==rs::LuaType::Table)
		const auto vt = v.toValue<std::vector<float>>();
		Vec2 dir(vt.at(0), vt.at(1));
		dir.normalize();
		_angle = spn::AngleValue(dir);
		_bRefl[Enum::Value] = true;
	}
	void Dir2D::increment(const float inc, const int index) {
		if(index == 0) {
			_angle += _step * inc;
			_angle.set(spn::rect::LoopValue(_angle.get(), spn::DegF::OneRotationAng));
			_bRefl[Enum::Value] = true;
		}
	}
	rs::HLText Dir2D::_getText(rs::CCoreID cid, const int t) const {
		if(t == Enum::Value)
			return MakeValueText(cid, _getRaw().m, 2);
		const float stp = _step.get();
		return MakeValueText(cid, &stp, 1);
	}
	spn::Vec2 Dir2D::_getRaw() const {
		return spn::VectorFromAngle(_angle);
	}
	rs::LCValue Dir2D::get() const {
		return _getRaw();
	}
	spn::SizeF Dir2D::draw(const Vec2& offset,  const Vec2& unit, Drawer& d) const {
		return DrawText(d, offset, unit, getText(Enum::Value), {Color::Dir2D});
	}
	spn::SizeF Dir2D::drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const {
		return DrawInfo(this, _stextD2.getText(Text::Name), offset, unit, st, d);
	}
	std::ostream& Dir2D::write(std::ostream& s) const {
		auto val = _getRaw();
		return s << '{' << val.x << ", " << val.y << '}';
	}
	const std::string Dir2D::Name("dir2d");
	
	// ---------------- Dir3D ----------------
	namespace {
		const std::string cs_d3Str[] = {
			"Dir3D"
		};
	}
	Dir3D::Dir3D(rs::CCoreID cid):
		ValueT(cid),
		_angle{DegF(0), DegF(0)},
		_step{DegF(1), DegF(1)},
		_stextD3(cid, countof(cs_d3Str), cs_d3Str)
	{}
	void Dir3D::loadDefine(const LValueS& tbl) {
		// Value
		loadValue(tbl[entry::Value]);
		// Step
		{
			const rs::LValueS v(tbl[entry::Step]);
			const auto typ = v.type();
			AssertP(Throw, typ==rs::LuaType::Table || typ==rs::LuaType::Number)
			if(typ == rs::LuaType::Number) {
				_step[0] = _step[1] = DegF(v.toNumber());
			} else {
				_step[0].set(LValueS(v[1]).toNumber());
				_step[1].set(LValueS(v[2]).toNumber());
			}
			_bRefl[Enum::Step] = true;
		}
	}
	void Dir3D::loadValue(const LValueS& v) {
		AssertP(Throw, v.type()==rs::LuaType::Table)
		const auto vt = v.toValue<std::vector<float>>();
		const auto ypd = spn::YawPitchDist::FromPos({vt.at(0), vt.at(1), vt.at(2)});
		_angle[0] = ypd.yaw;
		_angle[0].single();
		_angle[1] = ypd.pitch;
		_angle[1].single();
		_bRefl[Enum::Value] = true;
	}
	void Dir3D::increment(const float inc, const int index) {
		if(index < 2) {
			auto& d = _angle[index];
			d += _step[index] * inc;
			d.set(std::fmod(d.get(), spn::DegF::OneRotationAng));
			_bRefl[Enum::Value] = true;
		}
	}
	spn::Vec3 Dir3D::_getRaw() const {
		using spn::DegF;
		const auto q = spn::AQuat::RotationYPR(_angle[0], _angle[1], DegF(0));
		return q.getDir();
	}
	rs::LCValue Dir3D::get() const {
		return _getRaw();
	}
	rs::HLText Dir3D::_getText(rs::CCoreID cid, const int t) const {
		if(t == Enum::Value)
			return MakeValueText(cid, _getRaw().m, 3);
		return MakeValueText(cid, Vec2(_step[0].get(), _step[1].get()).m, 2);
	}
	spn::SizeF Dir3D::draw(const Vec2& offset,  const Vec2& unit, Drawer& d) const {
		return DrawText(d, offset, unit, getText(Enum::Value), {Color::Dir3D});
	}
	spn::SizeF Dir3D::drawInfo(const Vec2& offset, const Vec2& unit, const STextPack& st, Drawer& d) const {
		return DrawInfo(this, _stextD3.getText(Text::Name), offset, unit, st, d);
	}
	std::ostream& Dir3D::write(std::ostream& s) const {
		auto val = _getRaw();
		return s << '{' << val.x << ", " << val.y << ", " << val.z << '}';
	}
	const std::string Dir3D::Name("dir3d");
}
