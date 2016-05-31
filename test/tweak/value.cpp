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
	{
		for(auto& b : _bRefl)
			b = true;
	}
	rs::HText Value::getText(VStep::type t) const {
		if(_bRefl[t]) {
			_bRefl[t] = false;
			_text[t] = _getText(t);
		}
		return _text[t];
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
		int DrawVector(Drawer& d, const spn::Vec2& offset, rs::HText hText, Color t) {
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
	LinearValue::LinearValue(rs::CCoreID cid, const int n):
		base_t(cid),
		_data{
			{n,0},
			{n,1}
		}
	{}
	void LinearValue::set(const LValueS& v, const bool bStep) {
		const auto iS = static_cast<int>(bStep);
		auto& dst = _data[iS];
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
		_bRefl[iS] = true;
	}
	void LinearValue::increment(const float inc, const int index) {
		if(index < _data[VStep::Value].nValue) {
			_data[VStep::Value].value.m[index] += _data[VStep::Step].value.m[index] * inc;
			_bRefl[VStep::Value] = true;
		}
	}
	rs::HLText LinearValue::_getText(VStep::type t) const {
		return MakeValueText(_cid, _data[t].value.m, _data[t].nValue);
	}
	rs::HLText LinearValue::_getText(SText::type /*t*/) const {
		return mgr_text.createText(_cid, "Linear");
	}
	int LinearValue::draw(const Vec2& offset, const Vec2&, Drawer& d) const {
		return DrawVector(d, offset, getText(VStep::Value), {Color::Linear});
	}
	rs::LCValue LinearValue::get() const {
		return _data[VStep::Value].toLCValue();
	}
	std::ostream& LinearValue::write(std::ostream& s) const {
		return _data[VStep::Value].write(s);
	}
	
	// ---------------- LogValue ----------------
	void LogValue::increment(const float inc, const int index) {
		if(index < _data[VStep::Value].nValue) {
			_data[VStep::Value].value.m[index] += std::pow(1.2f, _data[VStep::Step].value.m[index]) * inc;
			_bRefl[VStep::Value] = true;
		}
	}
	rs::HLText LogValue::_getText(SText::type /*t*/) const {
		return mgr_text.createText(_cid, "Log");
	}
	Value_UP LogValue::clone() const {
		return std::make_unique<LogValue>(*this);
	}
	int LogValue::draw(const Vec2& offset, const Vec2&, Drawer& d) const {
		return DrawVector(d, offset, getText(VStep::Value), {Color::Log});
	}
	
	// ---------------- Dir2D ----------------
	Dir2D::Dir2D(rs::CCoreID cid):
		ValueT(cid),
		_angle(0),
		_step(1)
	{}
	void Dir2D::set(const LValueS& v, const bool bStep) {
		if(bStep) {
			AssertP(Throw, v.type()==rs::LuaType::Number)
			_step.set(v.toNumber());
			_bRefl[VStep::Step] = true;
		} else {
			AssertP(Throw, v.type()==rs::LuaType::Table)
			const auto vt = v.toValue<std::vector<float>>();
			Vec2 dir(vt.at(0), vt.at(1));
			dir.normalize();
			_angle = spn::AngleValue(dir);
			_bRefl[VStep::Value] = true;
		}
	}
	void Dir2D::increment(const float inc, const int index) {
		if(index == 0) {
			_angle += _step * inc;
			_angle.set(spn::rect::LoopValue(_angle.get(), spn::DegF::OneRotationAng));
			_bRefl[VStep::Value] = true;
		}
	}
	rs::HLText Dir2D::_getText(VStep::type /*t*/) const {
		const auto v = _getRaw();
		return MakeValueText(_cid, v.m, 2);
	}
	rs::HLText Dir2D::_getText(SText::type /*t*/) const {
		return mgr_text.createText(_cid, "Dir2D");
	}
	spn::Vec2 Dir2D::_getRaw() const {
		return spn::VectorFromAngle(_angle);
	}
	rs::LCValue Dir2D::get() const {
		return _getRaw();
	}
	int Dir2D::draw(const Vec2& offset,  const Vec2&, Drawer& d) const {
		return DrawVector(d, offset, getText(VStep::Value), {Color::Dir2D});
	}
	std::ostream& Dir2D::write(std::ostream& s) const {
		auto val = boost::get<Vec2>(get());
		return s << '{' << val.x << ", " << val.y << '}';
	}
	
	// ---------------- Dir3D ----------------
	Dir3D::Dir3D(rs::CCoreID cid):
		ValueT(cid),
		_angle{DegF(0), DegF(0)},
		_step{DegF(1), DegF(1)}
	{}
	void Dir3D::set(const LValueS& v, const bool bStep) {
		const auto typ = v.type();
		if(bStep) {
			AssertP(Throw, typ==rs::LuaType::Table || typ==rs::LuaType::Number)
			if(typ == rs::LuaType::Number) {
				_step[0] = _step[1] = DegF(v.toNumber());
			} else {
				_step[0].set(LValueS(v[1]).toNumber());
				_step[1].set(LValueS(v[2]).toNumber());
			}
			_bRefl[VStep::Value] = true;
		} else {
			AssertP(Throw, typ==rs::LuaType::Table)
			const auto vt = v.toValue<std::vector<float>>();
			const auto ypd = spn::YawPitchDist::FromPos({vt.at(0), vt.at(1), vt.at(2)});
			_angle[0] = ypd.yaw;
			_angle[0].single();
			_angle[1] = ypd.pitch;
			_angle[1].single();
			_bRefl[VStep::Step] = true;
		}
	}
	void Dir3D::increment(const float inc, const int index) {
		if(index < 2) {
			auto& d = _angle[index];
			d += _step[index] * inc;
			d.set(std::fmod(d.get(), spn::DegF::OneRotationAng));
			_bRefl[VStep::Value] = true;
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
	rs::HLText Dir3D::_getText(VStep::type /*t*/) const {
		const auto v = _getRaw();
		return MakeValueText(_cid, v.m, 3);
	}
	rs::HLText Dir3D::_getText(SText::type /*t*/) const {
		return mgr_text.createText(_cid, "Dir3D");
	}
	int Dir3D::draw(const Vec2& offset,  const Vec2&, Drawer& d) const {
		return DrawVector(d, offset, getText(VStep::Value), {Color::Dir3D});
	}
	std::ostream& Dir3D::write(std::ostream& s) const {
		auto val = boost::get<Vec3>(get());
		return s << '{' << val.x << ", " << val.y << ", " << val.z << '}';
	}
}
