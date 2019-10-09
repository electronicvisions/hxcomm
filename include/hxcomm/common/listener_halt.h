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
	/**
	 * Construct Halt listener.
	 */
	ListenerHalt() : m_value(false) {}

	/**
	 * Operator invoked for every decoded message checking whether the message contains a Halt.
	 * @tparam MessageType Type of UT message to check
	 * @param message UT message instance to check
	 */
	template <typename MessageType>
	void operator()(MessageType const& message)
	{
		if constexpr (std::is_same<MessageType, HaltMessageType>::value) {
			bool expected = false;
			m_value.compare_exchange_strong(
			    expected, (message.decode() == MessageType::instruction_type::halt));
		}
	}

	/**
	 * Get whether the listener registered a Halt message.
	 */
	bool get() const { return m_value.load(std::memory_order_acquire); }

	/**
	 * Reset listener state to not having registered a Halt message.
	 */
	void reset() { m_value = false; }

private:
	std::atomic<bool> m_value;
};

} // namespace hxcomm
