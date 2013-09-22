#pragma once
#include <SDL2/SDL_thread.h>
#include "spinner/misc.hpp"
#include "sdlwrap.hpp"
#include <functional>
#include <chrono>
#include <deque>
#include "spinner/optional.hpp"
#include "spinner/pqueue.hpp"

using Clock = std::chrono::steady_clock;
using Timepoint = typename Clock::time_point;
using Duration = typename Clock::duration;
namespace sdlw {
	extern SDL_threadID thread_local tls_threadID;
}

using Exec = std::function<void ()>;
class Handler;
class Looper;
using UPLooper = std::unique_ptr<Looper>;

struct Message;
using OPMessage = spn::Optional<Message>;

int GetNewMessageID();
struct MsgID {
	int id;
	operator int () const;
	bool operator == (const MsgID& m) const;
	bool operator < (const MsgID& m) const;
};
struct MsgBase {
	const static MsgID ID;
};

struct Message {
	const static Duration NoDelay;
	Timepoint	tpoint;
	MsgID		id;
	// IExecを呼び出すまでもないパラメータを渡す時に使用
	struct Data {
		constexpr static int PAYLOAD_SIZE = 32;
		uint8_t		data[PAYLOAD_SIZE];
		Data() = default;
		Data(const Data& d) {
			std::memcpy(this, &d, sizeof(*this));
		}
	};
	Data		data;
	Handler*	handler;
	Exec		exec;

	Message() = default;
	Message(const Message& m) = delete;
	Message(Message&& m);
	Message(Duration delay, Exec&& e);
	template <class T, class = typename std::is_base_of<MsgBase,T>::type>
	Message(Duration delay, const T& info, Exec&& e = Exec()): tpoint(Clock::now() + delay), id{info.ID}, handler(nullptr), exec(std::move(e)) {
		static_assert(sizeof(T) <= sizeof(Message::Data), "invalid payload size (T > Message_Default)");
		std::memcpy(&data, &info, sizeof(T));
	}

	void operator = (const Message& m) = delete;
	Message& operator = (Message&& m);
	bool operator < (const Message& m) const;

	template <class T,
			class = typename std::is_base_of<MsgBase,T>::type,
			class = typename std::enable_if<std::is_pointer<T>::value>::type>
	operator T () {
		using TR = typename std::remove_pointer<T>::type;
		if(id == TR::ID)
			return payload<TR>();
		return nullptr;
	}
	template <class T,
			class = typename std::is_base_of<MsgBase,T>::type,
			class = typename std::enable_if<std::is_pointer<T>::value>::type>
	operator const T () const {
		T t = const_cast<Message*>(this)->operator T();
		return (const T)t;
	}

	template <class T>
	T* payload() {
		static_assert(sizeof(T) <= sizeof(Message::Data), "invalid payload size (T > Message_Default)");
		return reinterpret_cast<T*>(&data);
	}
	template <class T>
	const T* payload() const {
		return const_cast<Message*>(this)->payload<T>();
	}
};

using MsgQueue = spn::pqueue<Message, std::deque>;
class Looper {
// 	static thread_local UPLooper tls_looper;
	static sdlw::TLS<Looper>	tls_looper;
	bool			_bRun = true;
	MsgQueue		_msg;
	sdlw::CondV		_cond;
	sdlw::Mutex		_mutex;

	OPMessage _procMessage(std::function<bool (sdlw::UniLock&)> lf);
	public:
		Looper() = default;
		Looper(const Looper&) = delete;
		Looper(Looper&& lp);
		Looper& operator = (Looper&& lp);
		// 呼び出したスレッドでキューがまだ用意されて無ければイベントキューを作成
		static void Prepare();
		// キューに溜まったメッセージをハンドラに渡す -> キューをクリア
		OPMessage wait();
		OPMessage peek(std::chrono::milliseconds msec);
		static Looper& GetLooper();
		void pushEvent(Message&& m);
		void setState(bool bRun);
};

class Handler {
	Looper&		_looper;

	friend class Looper;
	protected:
		virtual void handleMessage(const Message& msg);
	public:
		Handler(Looper& loop);
		Looper& getLooper() const;
		void post(Message&& m);
		template <class... Args>
		void postArgsDelay(Args&&... args) {
			post(Message(std::forward<Args>(args)...));
		}
		template <class... Args>
		void postArgs(Args&&... args) {
			post(Message(std::chrono::seconds(0), std::forward<Args>(args)...));
		}
};
