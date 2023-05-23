#include "hate/timer.h"
#include <yaml-cpp/yaml.h>

#include <chrono>

namespace hxcomm {

template <typename ConnectionParameter>
template <typename InputIterator>
void ARQConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to moved-from ARQConnection.");
	}
	m_encoder(begin, end);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

} // namespace hxcomm
