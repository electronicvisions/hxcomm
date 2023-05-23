#include "hate/timer.h"

namespace hxcomm {

template <typename ConnectionParameter>
template <typename InputIterator>
void ZeroMockConnection<ConnectionParameter>::add(
    InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	// reserve enough for common use case in execute_messages
	size_t const messages_size = std::distance(begin, end);
	m_receive_queue.reserve(m_receive_queue.size() + messages_size + 1 /* halt */);
	for (auto it = begin; it != end; ++it) {
		m_process_message(*it);
	}
	m_last_message_count += messages_size;
	std::chrono::nanoseconds duration(timer.get_ns());
	m_time_info.execution_duration += duration;
}

} // namespace hxcomm
