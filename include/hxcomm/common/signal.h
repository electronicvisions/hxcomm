#pragma once
#include <csignal>
#include <cstdlib>

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
	SignalOverrideIntTerm() :
	    m_previous_handler_sigint(signal(SIGINT, SIG_IGN)),
	    m_previous_handler_sigterm(signal(SIGTERM, SIG_IGN))
	{
		auto handler = [](int /*sig*/) { std::exit(EXIT_FAILURE); };

		// If the signal is ignored, for the lifetime change it to exiting on
		// {SIGINT,SIGTERM} to allow exiting the while loop below.
		if (m_previous_handler_sigint == SIG_IGN) {
			signal(SIGINT, handler);
		} else {
			signal(SIGINT, m_previous_handler_sigint);
		}

		if (m_previous_handler_sigterm == SIG_IGN) {
			signal(SIGTERM, handler);
		} else {
			signal(SIGTERM, m_previous_handler_sigterm);
		}
	}

	/**
	 * Destruct signal override with cleanup of signal handlers.
	 */
	~SignalOverrideIntTerm()
	{
		if (m_previous_handler_sigint == SIG_IGN) {
			signal(SIGINT, m_previous_handler_sigint);
		}
		if (m_previous_handler_sigterm == SIG_IGN) {
			signal(SIGTERM, m_previous_handler_sigterm);
		}
	}

private:
	typedef decltype(signal(SIGINT, SIG_IGN)) signal_handler_type;
	signal_handler_type m_previous_handler_sigint;
	signal_handler_type m_previous_handler_sigterm;
};

} // namespace hxcomm
