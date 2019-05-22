#include "hxcomm/common/signal.h"

#include <cstdlib>

namespace hxcomm {

SignalOverrideIntTerm::SignalOverrideIntTerm()
{
	m_previous_handler_sigint = signal(SIGINT, SIG_IGN);
	m_previous_handler_sigterm = signal(SIGTERM, SIG_IGN);
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

SignalOverrideIntTerm::~SignalOverrideIntTerm()
{
	if (m_previous_handler_sigint == SIG_IGN) {
		signal(SIGINT, m_previous_handler_sigint);
	}
	if (m_previous_handler_sigterm == SIG_IGN) {
		signal(SIGTERM, m_previous_handler_sigterm);
	}
}

} // namespace hxcomm
