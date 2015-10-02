#include "updater.hpp"

namespace rs {
	// -------------------- GMessage::Packet --------------------
	GMessage::Packet::Packet(const GMessageStr& str, const LCValue& args):
		msgStr(str),
		arg(args)
	{}
	GMessage::Packet::Packet(const GMessageStr& str, LCValue&& args):
		msgStr(str),
		arg(std::move(args))
	{}
	GMessage::Packet::Packet(Duration delay, const GMessageStr& str, const LCValue& args):
		Packet(str, args)
	{
		tmSend = Clock::now() + delay;
	}
	GMessage::Packet::Packet(Timepoint when, const GMessageStr& str, const LCValue& args):
		Packet(str, args)
	{
		tmSend = when;
	}
	void GMessage::Packet::swap(Packet& p) noexcept {
		std::swap(tmSend, p.tmSend);
		std::swap(msgStr, p.msgStr);
		std::swap(arg, p.arg);
	}
}

