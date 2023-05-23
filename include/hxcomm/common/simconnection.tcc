#include "hate/timer.h"

namespace hxcomm {

template <typename ConnectionParameter>
template <typename InputIterator>
void SimConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	m_encoder(begin, end);
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

} // namespace hxcomm
