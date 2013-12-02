#include "input.hpp"
#include "common.hpp"

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
	std::string Keyboard::s_name("(default keyboard)");
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
		return s_name;
	}

	// ----------------- Mouse -----------------
	InputType Mouse::getType() const {
		return InputType::Mouse;
	}
	int Mouse::getButton(int num) const {
		return dep_getButton(num);
	}
	int Mouse::getAxis(int num) const {
		auto& p = _hlPtr.cref();
		return _axisDZ[num].filter((num == 0) ? p.relPos.x : p.relPos.y);
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
		return name();
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

	// ----------------- InF -----------------
	namespace {
		int FM_Direct(int val) {
			return val;
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
	InF InF::AsButton(HInput hI, int num) {
		return InF(hI, num, &IInput::getButton, FM_Direct);
	}
	InF InF::AsAxis(HInput hI, int num) {
		return InF(hI, num, &IInput::getAxis, FM_Direct);
	}
	InF InF::AsAxisPositive(HInput hI, int num) {
		return InF(hI, num, &IInput::getAxis, FM_Positive);
	}
	InF InF::AsAxisNegative(HInput hI, int num) {
		return InF(hI, num, &IInput::getAxis, FM_Negative);
	}
	InF InF::AsHat(HInput hI, int num) {
		return InF(hI, num, &IInput::getHat, FM_Direct);
	}
	InF InF::AsHatX(HInput hI, int num) {
		return InF(hI, num, &IInput::getHat, FM_AngToX);
	}
	InF InF::AsHatY(HInput hI, int num) {
		return InF(hI, num, &IInput::getHat, FM_AngToY);
	}
	InF::InF(HInput hI, int num, FGet fGet, FManip fManip): _hlInput(hI), _num(num), _fGet(fGet), _fManip(fManip) {}
	int InF::getValue() const {
		IInput* iip = (_hlInput.get().ref().get());
		int val = (iip->*_fGet)(_num);
		return _fManip(val);
	}
	bool InF::operator == (const InF& f) const {
		return _hlInput == f._hlInput &&
				_num == f._num &&
				_fGet == f._fGet &&
				_fManip == f._fManip;
	}

	// ----------------- Action -----------------
	InputMgr::Action::Action(): _state(0), _value(0) {}
	InputMgr::Action::Action(Action&& a) noexcept: _link(std::move(a._link)), _state(a._state), _value(a._value) {}
	void InputMgr::Action::update() {
		int sum = 0;
		// linkの値を加算合成
		for(auto& l : _link)
			sum += l.getValue();
		sum = spn::Saturate(sum, InputRange);
		_advanceState(sum);
	}
	void InputMgr::Action::_advanceState(int val) {
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
	void InputMgr::Action::addLink(const InF& inF) {
		auto itr = std::find(_link.begin(), _link.end(), inF);
		if(itr == _link.end())
			_link.push_back(inF);
	}
	void InputMgr::Action::remLink(const InF& inF) {
		auto itr = std::find(_link.begin(), _link.end(), inF);
		if(itr != _link.end())
			_link.erase(itr);
	}
	int InputMgr::Action::getState() const {
		return _state;
	}
	int InputMgr::Action::getValue() const {
		return _value;
	}

	// ----------------- InputMgr -----------------
	bool InputMgr::isKeyPressed(HAct hAct) const {
		return _checkKeyValue([](int st){ return st==1; }, hAct);
	}
	bool InputMgr::isKeyReleased(HAct hAct) const {
		return _checkKeyValue([](int st){ return st==0; }, hAct);
	}
	bool InputMgr::isKeyPressing(HAct hAct) const {
		return _checkKeyValue([](int st){ return st>0; }, hAct);
	}
	int InputMgr::getKeyValue(HAct hAct) const {
		return hAct.ref().getValue();
	}

	InputMgr::HLAct InputMgr::addAction(const std::string& name) {
		HLAct ret = _act.acquire(name, Action()).first;
		_aset.insert(ret);
		return std::move(ret);
	}
	void InputMgr::link(HAct hAct, const InF& inF) {
		hAct.ref().addLink(inF);
	}
	void InputMgr::unlink(HAct hAct, const InF& inF) {
		hAct.ref().remLink(inF);
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
			Action& a = h.get().ref();
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
