#include "hate/timer.h"

namespace hxcomm {

template <typename ConnectionParameter>
template <typename InputIterator>
void ExtollConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}
	m_encoder(begin, end);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
}

} // namespace hxcomm
