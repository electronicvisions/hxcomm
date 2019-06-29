#pragma once
#include <atomic>

namespace hxcomm {

/**
 * Listener registering occurence of a halt instruction.
 * Filtering is done at compile-time for the halt message type and at runtime for the payload
 * comparing with the `halt` member of the messages instruction type.
 * @tparam HaltMessageType Message type of Halt instruction
 */
template <typename HaltMessageType>
class ListenerHalt
{
public:
	ListenerHalt() : m_value(false) {}

	template <typename MessageType>
	void operator()(MessageType const& message)
	{
		if constexpr (std::is_same<MessageType, HaltMessageType>::value) {
			bool expected = false;
			m_value.compare_exchange_strong(
			    expected, (message.decode() == MessageType::instruction_type::halt));
		}
	}

	size_t get() { return m_value.load(std::memory_order_acquire); }

	void reset() { m_value = false; }

private:
	std::atomic<bool> m_value;
};

} // namespace hxcomm
