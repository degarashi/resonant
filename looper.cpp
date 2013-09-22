#include "event.hpp"
#include "SDL2/SDL_thread.h"

const MsgID MsgBase::ID{GetNewMessageID()};
int GetNewMessageID() {
	static int s_messageID = 0;
	return s_messageID++;
}
bool MsgID::operator == (const MsgID& d) const {
	return id == d.id;
}
bool MsgID::operator < (const MsgID& d) const {
	return id < d.id;
}
MsgID::operator int () const { return id; }

// UPLooper thread_local Looper::tls_looper;
sdlw::TLS<Looper> Looper::tls_looper;

const Duration Message::NoDelay = std::chrono::seconds(0);
Message::Message(Message&& m): tpoint(m.tpoint), id(m.id), data(m.data),
		handler(m.handler), exec(std::move(m.exec)) {}
Message& Message::operator = (Message&& m) {
	tpoint = m.tpoint;
	id = m.id;
	data = m.data;
	handler = m.handler;
	exec = std::move(m.exec);
	return *this;
}
bool Message::operator < (const Message& m) const {
	return tpoint < m.tpoint;
}
Message::Message(Duration delay, Exec&& e):
	tpoint(Clock::now() + delay), id{-1}, exec(std::move(e)), handler(nullptr) {}

// ----------------- Looper -----------------
Looper::Looper(Looper&& lp): _msg(std::move(lp._msg)) {}
Looper& Looper::operator = (Looper&& lp) {
	std::swap(_msg, lp._msg);
	return *this;
}

void Looper::Prepare() {
	Assert(&tls_looper.get() == nullptr)
	tls_looper = Looper();
}

OPMessage Looper::_procMessage(std::function<bool (sdlw::UniLock&)> lf) {
	sdlw::UniLock lk(_mutex);
	auto tpoint = Clock::now();
	while(_bRun) {
		if(_msg.empty() ||
			tpoint < _msg.front().tpoint)
		{
			if(!lf(lk))
				break;
			tpoint = Clock::now();
		} else {
			Message m;
			_msg.pop_front(m);
			if(m.handler) {
				// ハンドラ持ちのメッセージはその場で実行
				m.handler->handleMessage(m);
			} else {
				// ユーザーに返して処理してもらう
				return std::move(m);
			}
		}
	}
	return spn::none;
}
OPMessage Looper::wait() {
	return _procMessage([this](sdlw::UniLock& lk) {
		if(_msg.empty())
			_cond.wait(lk);
		else {
			auto dur = _msg.front().tpoint - Clock::now();
			std::chrono::milliseconds msec(std::chrono::duration_cast<std::chrono::milliseconds>(dur));
			_cond.wait_for(lk, std::max<std::chrono::milliseconds::rep>(0, msec.count()));
		}
		return true;
	});
}
OPMessage Looper::peek(std::chrono::milliseconds msec) {
	return _procMessage([this, msec](sdlw::UniLock& lk) {
		return _cond.wait_for(lk, msec.count());
	});
}

void Looper::pushEvent(Message&& m) {
	sdlw::UniLock lk(_mutex);
	_msg.push(std::move(m));
	_cond.signal_all();
}
Looper& Looper::GetLooper() {
	return *tls_looper;
}
void Looper::setState(bool bRun) {
	sdlw::UniLock lk(_mutex);
	_bRun = bRun;
	if(!bRun)
		_cond.signal_all();
}

// ----------------- Handle -----------------
Handler::Handler(Looper& loop): _looper(loop) {}
void Handler::handleMessage(const Message& msg) {
	if(msg.exec)
		msg.exec();
}
void Handler::post(Message&& m) {
	m.handler = m.exec ? this : nullptr;
	_looper.pushEvent(std::move(m));
}
Looper& Handler::getLooper() const {
	return _looper;
}
