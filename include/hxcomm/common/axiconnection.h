#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <vector>

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/listener_halt.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/target.h"
#include "hxcomm/common/utmessage.h"

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace hxcomm {

class AXIHandle;

/**
 * AXI connection class.
 * Establish and hold AXI connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <typename ConnectionParameter>
class AXIConnection
{
public:
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	using init_parameters_type = typename std::tuple<>;

	static constexpr char name[] = "AXIConnection";

	typedef std::vector<receive_message_type> receive_queue_type;

	/**
	 * Create connection to FPGA via AXI.
	 */
	AXIConnection();

	/**
	 * Copy constructor (deleted because no two instances with the same hardware
	 * allocation can coexist).
	 */
	AXIConnection(AXIConnection const&) = delete;

	/**
	 * Assignment operator (deleted because no two instances with the same hardware
	 * allocation can coexist).
	 */
	AXIConnection& operator=(AXIConnection const&) = delete;

	/**
	 * Move constructor.
	 */
	AXIConnection(AXIConnection&& other);

	/**
	 * Assignment operator.
	 */
	AXIConnection& operator=(AXIConnection&& other);

	/**
	 * Destruct connection to FPGA joining all receive threads.
	 */
	~AXIConnection();

	constexpr static std::initializer_list<hxcomm::Target> supported_targets = {Target::hardware};

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
	friend Stream<AXIConnection>;
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
	 * All messages in the send queue are guaranteed to be transfered.
	 */
	void commit();

	/**
	 * Receive all UT messages currently in the receive queue.
	 * @return Received messages
	 */
	receive_queue_type receive_all();

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const;

	/**
	 * Start execution and wait until halt instruction is received from FPGA.
	 */
	void run_until_halt();

	/**
	 * Get internal mutex to use for mutual exclusion.
	 * @return Mutable reference to mutex
	 */
	std::mutex& get_mutex();

	std::unique_ptr<AXIHandle> m_axi;
	typedef typename ConnectionParameter::Send::PhywordType subpacket_type;

	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Send::PhywordType>::value,
	    "subpacket_type does not match send PhyWord type.");
	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Receive::PhywordType>::value,
	    "subpacket_type does not match receive PhyWord type.");

	typedef std::queue<subpacket_type> send_queue_type;
	send_queue_type m_send_queue;

	typedef Encoder<typename ConnectionParameter::Send, send_queue_type> encoder_type;
	encoder_type m_encoder;

	mutable std::mutex m_receive_queue_mutex;
	receive_queue_type m_receive_queue;

	typedef ListenerHalt<UTMessage<
	    ConnectionParameter::Receive::HeaderAlignment,
	    typename ConnectionParameter::Receive::SubwordType,
	    typename ConnectionParameter::Receive::PhywordType,
	    typename ConnectionParameter::Receive::Dictionary,
	    typename ConnectionParameter::ReceiveHalt>>
	    listener_halt_type;
	listener_halt_type m_listener_halt;

	typedef Decoder<typename ConnectionParameter::Receive, receive_queue_type, listener_halt_type>
	    decoder_type;
	decoder_type m_decoder;

	std::atomic<bool> m_run_receive;

	log4cxx::Logger* m_logger;

	void work_receive();
	std::thread m_worker_receive;

	std::mutex m_mutex;

	typedef std::atomic<std::chrono::nanoseconds::rep> duration_type;
	duration_type m_encode_duration{};
	duration_type m_decode_duration{};
	duration_type m_commit_duration{};
	duration_type m_execution_duration{};
};

} // namespace hxcomm

#include "hxcomm/common/axiconnection.tcc"
