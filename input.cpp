#include "input.hpp"

namespace rs {
	DZone::DZone(float r, float dz): ratio(r), deadzone(dz) {}
	int DZone::filter(int val) const {
		float fval = (val * ratio) / InputRange;
		if(val > 0)
			fval = std::max(0.f, fval-deadzone*ratio);
		else
			fval = std::min(0.f, fval+deadzone*ratio);
		return spn::Saturate(static_cast<int>(fval * InputRange), InputRange);
	}

	void RecvPtrGroup::newPointer(WPtr wptr) {
		for(auto* p : listener)
			p->newPointer(wptr);
	}

	namespace {
		template <class DST, class FUNC>
		void Proc(DST& dst, FUNC func) {
			int n = static_cast<int>(dst.size());
			for(int i=0 ; i<n ; i++)
				dst[n] = func(i);
		}
	}
	// ----------------- Keyboard -----------------
	InputType Keyboard::getType() const {
		return InputType::Keyboard;
	}
	int Keyboard::getButton(int num) const {
		return dep_getButton(num);
	}
	int Keyboard::numButtons() const {
		return NUM_VKEY;
	}
	HLInput Keyboard::OpenKeyboard() {
		return KeyboardDep::OpenKeyboard<Keyboard>();
	}
	bool Keyboard::scan() {
		return dep_scan();
	}
	const std::string& Keyboard::name() const {
		return KeyboardDep::name();
	}

	// ----------------- Mouse -----------------
	InputType Mouse::getType() const {
		return InputType::Mouse;
	}
	int Mouse::getButton(int num) const {
		return dep_getButton(num);
	}
	int Mouse::getAxis(int num) const {
		if(num < 2) {
			auto& p = _hlPtr.cref();
			return _axisDZ[num].filter((num == 0) ? p.relPos.x : p.relPos.y);
		}
		return _axisDZ[num].filter(dep_getAxis(num));
	}
	int Mouse::numButtons() const {
		return dep_numButtons();
	}
	int Mouse::numAxes() const {
		return dep_numAxes();
	}
	bool Mouse::scan() {
		return dep_scan(_hlPtr.ref());
	}
	HLInput Mouse::OpenMouse(int num) {
		return MouseDep::OpenMouse<Mouse>(num);
	}
	const std::string& Mouse::name() const {
		return MouseDep::name();
	}
	void Mouse::setMouseMode(MouseMode mode) {
		dep_setMode(mode, _hlPtr.ref());
	}
	MouseMode Mouse::getMouseMode() const {
		return dep_getMode();
	}
	WPtr Mouse::getPointer() const {
		// 常にマウスの座標を返す
		return _hlPtr.weak();
	}
	void Mouse::setDeadZone(int num, float r, float dz) {
		auto& d = _axisDZ[num];
		d.ratio = r;
		d.deadzone = dz;
	}

	// ----------------- Joypad -----------------
	InputType Joypad::getType() const {
		return InputType::Joypad;
	}
	int Joypad::getButton(int num) const {
		return dep_getButton(num);
	}
	int Joypad::getAxis(int num) const {
		return _axisDZ[num].filter(dep_getAxis(num));
	}
	int Joypad::getHat(int num) const {
		return dep_getHat(num);
	}
	int Joypad::numButtons() const {
		return dep_numButtons();
	}
	int Joypad::numAxes() const {
		return dep_numAxes();
	}
	int Joypad::numHats() const {
		return dep_numHats();
	}
	bool Joypad::scan() {
		return dep_scan();
	}
	HLInput Joypad::OpenJoypad(int num) {
		return JoypadDep::OpenJoypad<Joypad>(num);
	}
	const std::string& Joypad::name() const {
		return JoypadDep::name();
	}

	// ----------------- Touchpad -----------------
	InputType Touchpad::getType() const {
		return InputType::Touchpad;
	}
	bool Touchpad::scan() {
		return dep_scan(&_group);
	}
	void Touchpad::addListener(IRecvPointer* r) {
		_group.listener.insert(r);
	}
	void Touchpad::remListener(IRecvPointer* r) {
		_group.listener.erase(r);
	}
	HLInput Touchpad::OpenTouchpad(int num) {
		return TouchDep::OpenTouchpad<Touchpad>(num);
	}
	const std::string& Touchpad::name() const {
		return TouchDep::name();
	}
	WPtr Touchpad::getPointer() const {
		return dep_getPointer();
	}

	// ----------------- Action::Funcs -----------------
	namespace detail {
		namespace {
			int FM_Direct(int val) {
				return val;
			}
			int FM_Flip(int val) {
				return -val;
			}
			int FM_Positive(int val) {
				return std::max(0, val);
			}
			int FM_Negative(int val) {
				return std::max(0, -val);
			}
			template <class F>
			int16_t HatAngToValue(int val, F f) {
				if(val == -1)
					return int16_t(0);
				auto ang = static_cast<float>(val) / static_cast<float>(InputRange);
				ang *= 2*spn::PI;
				return static_cast<int16_t>(f(ang) * InputRange);
			}
			int FM_AngToX(int val) {
				using SF = float (*)(float);
				return HatAngToValue<SF>(val, std::sin);
			}
			int FM_AngToY(int val) {
				using SF = float (*)(float);
				return HatAngToValue<SF>(val, std::cos);
			}
		}
		const Action::Funcs Action::cs_funcs[InputFlag::_Num] = {
			{&IInput::getButton, FM_Direct},
			{&IInput::getButton, FM_Flip},
			{&IInput::getAxis, FM_Direct},
			{&IInput::getAxis, FM_Positive},
			{&IInput::getAxis, FM_Negative},
			{&IInput::getHat, FM_Direct},
			{&IInput::getHat, FM_AngToX},
			{&IInput::getHat, FM_AngToY}
		};

		// ----------------- Action::Link -----------------
		int Action::Link::getValue() const {
			auto f = cs_funcs[inF];
			IInput* iip = (hlInput.get().ref().get());
			int val = (iip->*f.getter)(num);
			return f.manipulator(val);
		}
		bool Action::Link::operator == (const Link& l) const {
			return hlInput == l.hlInput &&
					num == l.num &&
					inF == l.inF;
		}
		// ----------------- Action -----------------
		Action::Action(): _state(0), _value(0) {}
		Action::Action(Action&& a) noexcept: _link(std::move(a._link)), _state(a._state), _value(a._value) {}
		void Action::update() {
			int sum = 0;
			// linkの値を加算合成
			for(auto& l : _link)
				sum += l.getValue();
			sum = spn::Saturate(sum, InputRange);
			_advanceState(sum);
		}
		void Action::_advanceState(int val) {
			if(val >= InputRangeHalf) {
				if(_state <= 0)
					_state = 1;
				else {
					// オーバーフロー対策
					_state = std::max(_state+1, _state);
				}
			} else {
				if(_state > 0)
					_state = 0;
				else {
					// アンダーフロー対策
					_state = std::min(_state-1, _state);
				}
			}
			_value = val;
		}
	bool detail::Action::isKeyPressed() const {
		return getState() == 1;
	}
	bool detail::Action::isKeyReleased() const {
		return getState() == 0;
	}
	bool detail::Action::isKeyPressing() const {
		return getState() > 0;
	}
		void Action::addLink(HInput hI, InputFlag::E inF, int num) {
			Link link{hI, inF, num};
			auto itr = std::find(_link.begin(), _link.end(), link);
			if(itr == _link.end())
				_link.emplace_back(link);
		}
		void Action::remLink(HInput hI, InputFlag::E inF, int num) {
			Link link{hI, inF, num};
			auto itr = std::find(_link.begin(), _link.end(), link);
			if(itr != _link.end())
				_link.erase(itr);
		}
		int Action::getState() const {
			return _state;
		}
		int Action::getValue() const {
			return _value;
		}
	}
	// ----------------- ActMgr -----------------
	const std::string& detail::ActMgr::getResourceName(spn::SHandle /*sh*/) const {
		static std::string name("Action");
		return name;
	}
	// ----------------- InputMgr -----------------
	const std::string& InputMgrBase::getResourceName(spn::SHandle /*sh*/) const {
		static const std::string str("IInput");
		return str;
	}
	HLAct InputMgr::makeAction(const std::string& name) {
		HLAct ret = _act.acquire(name, [](const auto&){ return detail::Action(); }).first;
		_aset.insert(ret);
		return ret;
	}
	void InputMgr::linkButtonAsAxis(HInput hI, HAct hAct, int num_negative, int num_positive) {
		hAct->addLink(hI, InputFlag::ButtonFlip, num_negative);
		hAct->addLink(hI, InputFlag::Button, num_positive);
	}
	int InputMgr::getKeyValueSimplified(HAct hAct) {
		auto v = hAct->getValue();
		if(v >= InputRangeHalf)
			return 1;
		if(v <= -InputRangeHalf)
			return -1;
		return 0;
	}
	HLInput InputMgr::addInput(UPInput&& u) {
		return acquire(std::move(u));
	}
	void InputMgr::addAction(HAct hAct) {
		_aset.insert(HLAct(hAct));
	}
	void InputMgr::remAction(HAct hAct) {
		_aset.erase(HLAct(hAct));
	}
	namespace {
		// Terminateメソッドを持っていたら呼び出す
		#define DEF_CALLER(func) \
			struct Call##func { \
				template <class T> \
				void operator()(...) const {} \
				template <class T> \
				void operator()(decltype(&T::func)=nullptr) const { \
					T::func(); \
				} \
		};

		DEF_CALLER(Initialize)
		DEF_CALLER(Terminate)
		DEF_CALLER(Update)
		#undef DEF_CALLER

		template <class Caller>
		void CallFunction2(Caller) {}
		template <class Caller, class T, class... Ts>
		void CallFunction2(Caller c, T*, Ts*...) {
			c.template operator()<T>(nullptr);
			CallFunction2(c, (Ts*)(nullptr)...);
		}
		template <class... Ts, class Caller>
		void CallFunction(Caller c) {
			CallFunction2(c, (Ts*)(nullptr)...);
		}
	}
	void InputMgr::update() {
		CallFunction<Keyboard, Mouse, Joypad, Touchpad>(CallUpdate());
		for(auto& h : *this)
			h->scan();
		for(auto& h : _aset) {
			detail::Action& a = h.get().ref();
			a.update();
		}
	}
	InputMgr::InputMgr() {
		CallFunction<Keyboard, Mouse, Joypad, Touchpad>(CallInitialize());
	}
	InputMgr::~InputMgr() {
		CallFunction<Keyboard, Mouse, Joypad, Touchpad>(CallTerminate());
	}
}
