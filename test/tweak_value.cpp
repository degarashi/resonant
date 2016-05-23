#include "spinner/structure/angle.hpp"
namespace spn {
	template <class V, class T>
	constexpr T Angle<V, T>::OneRotationAng;
}
#include "tweak.hpp"
#include "spinner/rectdiff.hpp"

// ---------------- FVec4 ----------------
FVec4::FVec4(const int nv, const float iv):
	nValue(nv),
	value(iv)
{}
FVec4 FVec4::LoadFromLua(const rs::LValueS& v) {
	FVec4 ret;
	auto& value = ret.value;
	int cur = 0;
	auto fn = [&v, &cur](auto& dst, const auto& key){
		const rs::LValueS val = v[key];
		if(val.type() == rs::LuaType::Number) {
			++cur;
			dst = val.template toValue<float>();
			return true;
		}
		return false;
	};
	if(v.type() == rs::LuaType::Number) {
		value.x = v.toValue<float>();
		cur = 1;
	} else if(fn(value.m[0], rs::LCValue(1))) {
		if(fn(value.m[1], 2))
			if(fn(value.m[2], 3))
				fn(value.m[3], 4);
	} else {
		fn(value.x, "x");
		if(fn(value.y, "y"))
			if(fn(value.z, "z"))
				fn(value.w, "w");
	}
	ret.nValue = cur;
	return ret;
}
rs::LCValue	FVec4::toLCValue() const {
	switch(nValue) {
		case 2: return spn::Vec2(value.x, value.y);
		case 3: return spn::Vec3(value.x, value.y, value.z);
		case 4: return value;
	}
	return value.x;
}

// ---------------- IBase ----------------
void Tweak::IBase::set(const LValueS&, bool) {
	Assert(Warn, false, "invalid function call")
}
void Tweak::IBase::increment(float,int) {
	Assert(Warn, false, "invalid function call")
}
int Tweak::IBase::draw(const Vec2&,  const Vec2&, Drawer&) const {
	AssertF(Trap, "invalid function call")
}
rs::LCValue Tweak::IBase::get() const {
	Assert(Warn, false, "invalid function call")
	return rs::LCValue();
}

// ---------------- Value ----------------
Tweak::Value::Value(rs::CCoreID cid):
	_cid(cid),
	_bRefl(true)
{}
rs::HText Tweak::Value::getValueText() const {
	if(_bRefl) {
		_bRefl = false;
		_valueText = _getValueText();
	}
	return _valueText;
}
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
	int DrawVector(Tweak::Drawer& d, const spn::Vec2& offset, rs::HText hText, Tweak::Color t) {
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
		return 1;
	}
}
// ---------------- LinearValue ----------------
Tweak::LinearValue::LinearValue(rs::CCoreID cid, const int n):
	base_t(cid),
	_value(n, 0),
	_step(n, 1)
{}
void Tweak::LinearValue::set(const LValueS& v, const bool bStep) {
	auto& dst = bStep ? _step: _value;
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
	_bRefl = true;
}
void Tweak::LinearValue::increment(const float inc, const int index) {
	if(index < _value.nValue) {
		_value.value.m[index] += _step.value.m[index] * inc;
		_bRefl = true;
	}
}
rs::HLText Tweak::LinearValue::_getValueText() const {
	return MakeValueText(_cid, _value.value.m, _value.nValue);
}
int Tweak::LinearValue::draw(const Vec2& offset, const Vec2&, Drawer& d) const {
	return DrawVector(d, offset, getValueText(), {Color::Linear});
}
rs::LCValue Tweak::LinearValue::get() const {
	return _value.toLCValue();
}

// ---------------- LogValue ----------------
void Tweak::LogValue::increment(const float inc, const int index) {
	if(index < _value.nValue) {
		_value.value.m[index] += std::pow(1.2f, _step.value.m[index]) * inc;
		_bRefl = true;
	}
}
Tweak::Value_UP Tweak::LogValue::clone() const {
	return std::make_unique<LogValue>(*this);
}
int Tweak::LogValue::draw(const Vec2& offset, const Vec2&, Drawer& d) const {
	return DrawVector(d, offset, getValueText(), {Color::Log});
}

// ---------------- Dir2D ----------------
Tweak::Dir2D::Dir2D(rs::CCoreID cid):
	ValueT(cid),
	_angle(0),
	_step(1)
{}
void Tweak::Dir2D::set(const LValueS& v, const bool bStep) {
	if(bStep) {
		AssertP(Throw, v.type()==rs::LuaType::Number)
		_step.set(v.toNumber());
	} else {
		AssertP(Throw, v.type()==rs::LuaType::Table)
		const auto vt = v.toValue<std::vector<float>>();
		Vec2 dir(vt.at(0), vt.at(1));
		dir.normalize();
		_angle = spn::AngleValue(dir);
		_bRefl = true;
	}
}
void Tweak::Dir2D::increment(const float inc, const int index) {
	if(index == 0) {
		_angle += _step * inc;
		_angle.set(spn::rect::LoopValue(_angle.get(), spn::DegF::OneRotationAng));
		_bRefl = true;
	}
}
rs::HLText Tweak::Dir2D::_getValueText() const {
	const auto v = _getRaw();
	return MakeValueText(_cid, v.m, 2);
}
spn::Vec2 Tweak::Dir2D::_getRaw() const {
	return spn::VectorFromAngle(_angle);
}
rs::LCValue Tweak::Dir2D::get() const {
	return _getRaw();
}
int Tweak::Dir2D::draw(const Vec2& offset,  const Vec2&, Drawer& d) const {
	return DrawVector(d, offset, getValueText(), {Color::Dir2D});
}

// ---------------- Dir3D ----------------
Tweak::Dir3D::Dir3D(rs::CCoreID cid):
	ValueT(cid),
	_angle{DegF(0), DegF(0)},
	_step{DegF(1), DegF(1)}
{}
void Tweak::Dir3D::set(const LValueS& v, const bool bStep) {
	const auto typ = v.type();
	if(bStep) {
		AssertP(Throw, typ==rs::LuaType::Table || typ==rs::LuaType::Number)
		if(typ == rs::LuaType::Number) {
			_step[0] = _step[1] = DegF(v.toNumber());
		} else {
			_step[0].set(LValueS(v[1]).toNumber());
			_step[1].set(LValueS(v[2]).toNumber());
		}
	} else {
		AssertP(Throw, typ==rs::LuaType::Table)
		const auto vt = v.toValue<std::vector<float>>();
		const auto ypd = spn::YawPitchDist::FromPos({vt.at(0), vt.at(1), vt.at(2)});
		_angle[0] = ypd.yaw;
		_angle[0].single();
		_angle[1] = ypd.pitch;
		_angle[1].single();
		_bRefl = true;
	}
}
void Tweak::Dir3D::increment(const float inc, const int index) {
	if(index < 2) {
		auto& d = _angle[index];
		d += _step[index] * inc;
		d.set(std::fmod(d.get(), spn::DegF::OneRotationAng));
		_bRefl = true;
	}
}
spn::Vec3 Tweak::Dir3D::_getRaw() const {
	using spn::DegF;
	const auto q = spn::AQuat::RotationYPR(_angle[0], _angle[1], DegF(0));
	return q.getDir();
}
rs::LCValue Tweak::Dir3D::get() const {
	return _getRaw();
}
rs::HLText Tweak::Dir3D::_getValueText() const {
	const auto v = _getRaw();
	return MakeValueText(_cid, v.m, 3);
}
int Tweak::Dir3D::draw(const Vec2& offset,  const Vec2&, Drawer& d) const {
	return DrawVector(d, offset, getValueText(), {Color::Dir3D});
}
