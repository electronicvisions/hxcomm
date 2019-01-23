#pragma once
#include <atomic>

namespace hxcomm {

/**
 * Listener registering occurence of a halt instruction.
 */
template <typename HaltMessageType>
class ListenerHalt
{
public:
	ListenerHalt() : m_value(false) {}

	template <typename MessageType>
	void operator()(MessageType const& /*t*/)
	{
		m_value = m_value || std::is_same<MessageType, HaltMessageType>::value;
	}

	size_t get() { return m_value.load(std::memory_order_acquire); }

	void reset() { m_value = false; }

private:
	std::atomic<bool> m_value;
};

} // namespace hxcomm
