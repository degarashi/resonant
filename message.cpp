#include "updater.hpp"

namespace rs {
	// -------------------- GMessage::Packet --------------------
	GMessage::Packet::Packet(GMessageId id, const LCValue& args):
		msgId(id),
		arg(args)
	{}
	GMessage::Packet::Packet(GMessageId id, LCValue&& args):
		msgId(id),
		arg(std::move(args))
	{}
	GMessage::Packet::Packet(Duration delay, GMessageId id, const LCValue& args):
		Packet(id, args)
	{
		tmSend = Clock::now() + delay;
	}
	GMessage::Packet::Packet(Timepoint when, GMessageId id, const LCValue& args):
		Packet(id, args)
	{
		tmSend = when;
	}
	void GMessage::Packet::swap(Packet& p) noexcept {
		std::swap(tmSend, p.tmSend);
		std::swap(msgId, p.msgId);
		std::swap(arg, p.arg);
	}
	
	// -------------------- GMessage --------------------
	GMessage::GMessage():
		_msgIdCur(0)
	{}
	GMessage& GMessage::Ref() {
		static GMessage m;
		return m;
	}
	GMessageId GMessage::RegMsgId(const GMessageStr& msg) {
		GMessage& m = Ref();
		auto itr = m._msgMap.find(msg);
		if(itr != m._msgMap.end())
			return itr->second;
		auto id = m._msgIdCur++;
		m._msgMap.insert(std::make_pair(msg, id));
		return id;
	}
	spn::Optional<GMessageId> GMessage::GetMsgId(const GMessageStr& msg) {
		GMessage& m = Ref();
		auto itr = m._msgMap.find(msg);
		if(itr == m._msgMap.end())
			return spn::none;
		return itr->second;
	}
}

