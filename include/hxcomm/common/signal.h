#pragma once
#include <csignal>

namespace hxcomm {

/**
 * Class overriding the SIGINT and the SIGTERM signal handler to a exit handler during life-time.
 */
class SignalOverrideIntTerm
{
public:
	/**
	 * Construct signal override for SIGINT and SIGTERM by alteration to a exit handler.
	 */
	SignalOverrideIntTerm();

	/**
	 * Destruct signal override with cleanup of signal handlers.
	 */
	~SignalOverrideIntTerm();

private:
	typedef decltype(signal(SIGINT, SIG_IGN)) signal_handler_type;
	signal_handler_type m_previous_handler_sigint;
	signal_handler_type m_previous_handler_sigterm;
};

} // namespace hxcomm
