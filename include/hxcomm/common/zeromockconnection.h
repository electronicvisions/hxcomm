#pragma once
#include <mutex>
#include <vector>

#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/target.h"
#include "hxcomm/common/utmessage.h"

namespace hxcomm {

namespace detail {

/**
 * Process message implementation used in ZeroMockConnection.
 * A specialization is necessary for each connection parameter set.
 * @tparam ConnectionParameter Connection parameter for which to process messages
 */
template <typename ConnectionParameter>
struct ZeroMockProcessMessage;

} // namespace detail

/**
 * Connection returning zero'ed data on read requests, discarding all other messages.
 * We do not model experiment prebuffering, i.e. only message processing contributes
 * to the `execution_duration` measurement.
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <typename ConnectionParameter>
class ZeroMockConnection
{
public:
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	static constexpr char name[] = "ZeroMockConnection";

	/**
	 * Construct zero mock connection.
	 * The time spent with processing each incoming message can be adjusted to simulate different
	 * data communication speeds.
	 * @param ns_per_message Time goal to spent with processing each incoming message.
	 *                       The specified goal may not be reached as it is only an upper bound to
	 *                       the rate of processed messages and the actually reachable performance
	 *                       depends on the used host computer and its load.
	 */
	ZeroMockConnection(long ns_per_message = 8 /* ns/Message for 125M Messages/s */);

	ZeroMockConnection(ZeroMockConnection const&) = delete;
	ZeroMockConnection& operator=(ZeroMockConnection const&) = delete;
	ZeroMockConnection(ZeroMockConnection&& other);
	ZeroMockConnection& operator=(ZeroMockConnection&& other);
	~ZeroMockConnection() = default;

	constexpr static auto supported_targets = {Target::hardware, Target::simulation};
	typedef std::vector<receive_message_type> receive_queue_type;

	/**
	 * Get time information.
	 * @return Time information
	 */
	ConnectionTimeInfo get_time_info() const;

	/**
	 * Get unique identifier from hwdb.
	 * @param hwdb_path Optional path to hwdb
	 * @return Unique identifier
	 */
	std::string get_unique_identifier(std::optional<std::string> hwdb_path = std::nullopt) const;

private:
	friend Stream<ZeroMockConnection>;
	/**
	 * Add a single UT message to the send queue.
	 * @param message Message to add
	 */
	void add(send_message_type const& message);

	/**
	 * Add multiple UT messages to the send queue.
	 * @tparam InputIterator Iterator type to sequence of messages to add
	 * @param begin Iterator to beginning of sequence
	 * @param end Iterator to end of sequence
	 */
	template <typename InputIterator>
	void add(InputIterator const& begin, InputIterator const& end);

	/**
	 * Send messages in send queue.
	 */
	void commit();

	/**
	 * Receive all UT messages.
	 * @return Received messages
	 */
	receive_queue_type receive_all();

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const;

	/**
	 * Start execution and wait until halt instruction.
	 */
	void run_until_halt();

	/**
	 * Get internal mutex to use for mutual exclusion.
	 * @return Mutable reference to mutex
	 */
	std::mutex& get_mutex();

	typedef std::vector<send_message_type> send_queue_type;
	send_queue_type m_send_queue;
	receive_queue_type m_receive_queue;

	bool m_halt;
	long m_ns_per_message;

	detail::ZeroMockProcessMessage<ConnectionParameter> m_process_message;
	std::mutex m_mutex;

	ConnectionTimeInfo m_time_info;
	ConnectionTimeInfo m_last_time_info;
	/**
	 * Message count since last `run_until_halt()` invokation used to calculate the amount of
	 * time to spend in the current invokation in order to match specified rate of processed UT
	 * messages.
	 */
	size_t m_last_message_count;
};

} // namespace hxcomm

#include "hxcomm/common/zeromockconnection.tcc"
