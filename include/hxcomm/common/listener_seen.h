#pragma once
#include <atomic>
#include <type_traits>

namespace hxcomm {

/**
 * Listener registering occurence of a specific message type with any payload.
 * Filtering is done at compile-time for the templated message type.
 * Payload is not looked at.
 * @see ListenerHalt
 * @tparam SeenInstructionType Message type that should be checked for.
 */
template <typename SeenInstructionType>
class ListenerSeen
{
public:
	using seen_instruction_type = SeenInstructionType;

	/**
	 * Construct Seen listener.
	 */
	ListenerSeen() : m_seen{false} {}
	ListenerSeen(ListenerSeen const&) = default;
	ListenerSeen(ListenerSeen&&) = default;

	/**
	 * Operator invoked for every decoded message checking whether the message contains a Timeout.
	 * @tparam MessageType Type of UT message to check
	 * @param message UT message instance to check
	 */
	template <typename MessageType>
	void operator()(MessageType const&)
	{
		if constexpr (std::is_same_v<
		                  typename std::remove_cvref_t<MessageType>::instruction_type,
		                  seen_instruction_type>) {
			m_seen.store(true);
		}
	}

	/**
	 * Get whether the listener registered the temnplated message type.
	 */
	bool get() const
	{
		return m_seen.load(std::memory_order_acquire);
	}

	/**
	 * Reset listener state to not having registered a templated message type.
	 */
	void reset()
	{
		m_seen = false;
	}

private:
	std::atomic<bool> m_seen;
};

} // namespace hxcomm
