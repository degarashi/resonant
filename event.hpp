#pragma once
#include <SDL2/SDL_thread.h>
#include "spinner/misc.hpp"
#include "sdlwrap.hpp"
#include <functional>
#include <deque>
#include "spinner/optional.hpp"
#include "spinner/pqueue.hpp"
#include "clock.hpp"

namespace rs {
	extern SDL_threadID thread_local tls_threadID;

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
	class Looper;
	using SPLooper = std::shared_ptr<Looper>;
	using WPLooper = std::weak_ptr<Looper>;
	class Looper : public std::enable_shared_from_this<Looper> {
	// 	static thread_local SPLooper tls_looper;
		static TLS<SPLooper>	tls_looper;
		bool			_bRun = true;
		MsgQueue		_msg;
		CondV			_cond;
		Mutex			_mutex;

		OPMessage _procMessage(std::function<bool (UniLock&)> lf);
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
			static const SPLooper& GetLooper();
			void pushEvent(Message&& m);
			void setState(bool bRun);
	};

	class Handler {
		using Callback = std::function<void ()>; //MEMO Message&を渡しても良いかもしれない
		WPLooper	_looper;
		Callback	_cb;

		friend class Looper;
		void _callback();
		protected:
			virtual void handleMessage(const Message& msg);
		public:
			Handler(Handler&& h);
			Handler(const WPLooper& loop, Callback cb=Callback());
			const WPLooper& getLooper() const;
			void post(Message&& m);
			template <class... Args>
			void postArgsDelay(Args&&... args) {
				_callback();
				post(Message(std::forward<Args>(args)...));
			}
			template <class... Args>
			void postArgs(Args&&... args) {
				_callback();
				post(Message(std::chrono::seconds(0), std::forward<Args>(args)...));
			}
	};

	//! Looper付きスレッド
	template <class T>
	class ThreadL;
	template <class RET, class... Args>
	class ThreadL<RET (Args...)> : public Thread<RET (Args...)> {
		private:
			using base = Thread<RET (Args...)>;
			SPLooper	_spLooper;
		protected:
			using base::base;
			RET run(Args... args) override final {
				Looper::Prepare();
				_spLooper = Looper::GetLooper();
				return runL(std::forward<Args>(args)...);
			}
			virtual RET runL(Args... args) = 0;
		public:
			bool interrupt() override {
				if(base::interrupt()) {
					_spLooper->setState(false);
					return true;
				}
				return false;
			}
			const SPLooper& getLooper() {
				return _spLooper;
			}
	};
}
