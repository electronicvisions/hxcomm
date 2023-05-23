#pragma once
#include "bss_hw_params/cube_ethernet/constants.h"
#include "hate/visibility.h"
#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_registry.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/listener_halt.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/target.h"
#include "hxcomm/common/utmessage.h"
#include "sctrltp/ARQFrame.h"
#include "sctrltp/ARQStream.h"
#include <array>
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <tuple>
#include <vector>

namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
} // namespace log4cxx

namespace hxcomm {

/**
 * HostARQ connection class.
 * Establish and hold HostARQ connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <typename ConnectionParameter>
class ARQConnection
{
public:
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	using init_parameters_type = typename std::tuple<ip_t>;

	static constexpr char name[] = "ARQConnection";

	typedef std::vector<receive_message_type> receive_queue_type;

	/**
	 * Create connection to FPGA with IP address found in environment.
	 * @throws std::runtime_error On no or more than one FPGA IP address available in environment
	 */
	ARQConnection() SYMBOL_VISIBLE;

	/**
	 * Create connection to FPGA.
	 * @param ip IP-address of FPGA
	 */
	ARQConnection(ip_t ip) SYMBOL_VISIBLE;

	/**
	 * Copy constructor (deleted because no two instances with the same hardware
	 * allocation can coexist).
	 */
	ARQConnection(ARQConnection const&) = delete;

	/**
	 * Assignment operator (deleted because no two instances with the same hardware
	 * allocation can coexist).
	 */
	ARQConnection& operator=(ARQConnection const&) = delete;

	/**
	 * Move constructor.
	 */
	ARQConnection(ARQConnection&& other) SYMBOL_VISIBLE;

	/**
	 * Assignment operator.
	 */
	ARQConnection& operator=(ARQConnection&& other) SYMBOL_VISIBLE;

	/**
	 * Destruct connection to FPGA joining all receive threads.
	 */
	~ARQConnection();

	constexpr static auto supported_targets = {Target::hardware};

	/**
	 * Get time information.
	 * @return Time information
	 */
	ConnectionTimeInfo get_time_info() const SYMBOL_VISIBLE;

	/**
	 * Get unique identifier from hwdb.
	 * @param hwdb_path Optional path to hwdb
	 * @return Unique identifier
	 */
	std::string get_unique_identifier(std::optional<std::string> hwdb_path = std::nullopt) const
	    SYMBOL_VISIBLE;

	/**
	 * Get bitfile information.
	 * @return Bitfile info
	 */
	std::string get_bitfile_info() const SYMBOL_VISIBLE;

	/**
	 * Get server-side remote repository state information.
	 * Only non-empty for QuiggeldyConnection.
	 * @return Repository state
	 */
	std::string get_remote_repo_state() const SYMBOL_VISIBLE;

private:
	/**
	 * Registry of open ARQConnections.
	 */
	typedef ConnectionRegistry<ARQConnection> Registry;
	std::unique_ptr<Registry> m_registry;

	friend Stream<ARQConnection>;
	/**
	 * Add a single UT message to the send queue.
	 * Messages may be sent to the hardware as part of this call if the send queue is full.
	 * @param message Message to add
	 */
	void add(send_message_type const& message) SYMBOL_VISIBLE;

	/**
	 * Add multiple UT messages to the send queue.
	 * Messages may be sent to the hardware as part of this call if the send queue is full.
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
	void commit() SYMBOL_VISIBLE;

	/**
	 * Receive all UT messages currently in the receive queue.
	 * @return Received messages
	 */
	receive_queue_type receive_all() SYMBOL_VISIBLE;

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const SYMBOL_VISIBLE;

	/**
	 * Start execution and wait until halt instruction is received from FPGA.
	 */
	void run_until_halt() SYMBOL_VISIBLE;

	/**
	 * Get internal mutex to use for mutual exclusion.
	 * @return Mutable reference to mutex
	 */
	std::mutex& get_mutex() SYMBOL_VISIBLE;

	/**
	 * Check for bitfile version compatibility.
	 */
	void check_compatibility() const;

	/**
	 * Numbers to compare against bitfile protocol version
	 *
	 * There are 5 scenarios (newer covers new changes as well as outdated partners)
	 * 1: Software is newer than biftile and still compatible
	 * 2: Software is newer than biftile but incompatible
	 * 3: FPGA bitfile is newer than software and still compatible
	 * 4: FPGA bitfile is newer than software but incompatible
	 * 5: Both are head
	 *
	 * The FPGA returns two numbers
	 * - version: Integer automatically incrementing with each change to the bitfile
	 * - compatible_until: Integer manually incrementing when bitfile introduces breaking feature
	 *
	 * We can now cover all 5 cases by comparing these two numbers as follows
	 * - oldest_supported_version: manually set to last known bitfile version with breaking change
	 * - newest_supported_compatible_until: identical to current head bitfile compatible_until
	 *
	 * oldest_supported_version needs to be lesser or equal to version and
	 * newest_supported_compatible_until needs to be greater or equal to compatible_until
	 * Cases 1, 3 and 5 fulfill these conditions whereas 2 and 4 do not
	 */
	static constexpr size_t oldest_supported_version = 0;
	static constexpr size_t newest_supported_compatible_until =
	    bss_hw_params::cube_ethernet::bitfile_compatible_until;

	static constexpr uint16_t pid = 0x0010; // HostARQ UT packet type
	typedef sctrltp::ARQStream<sctrltp::ParametersFcpBss2Cube> arq_stream_type;
	std::unique_ptr<arq_stream_type> m_arq_stream;

	typedef sctrltp::packet<sctrltp::ParametersFcpBss2Cube>::entry_t subpacket_type;

	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Send::PhywordType>::value,
	    "HostARQ entry_t does not match send PhyWord type.");
	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Receive::PhywordType>::value,
	    "HostARQ entry_t does not match receive PhyWord type.");

	struct SendQueue
	{
	public:
		SendQueue(arq_stream_type& arq_stream);

		void push(subpacket_type const& subpacket);

		void flush();

	private:
		arq_stream_type& m_arq_stream;
		sctrltp::packet<sctrltp::ParametersFcpBss2Cube> m_packet;
	};

	typedef SendQueue send_queue_type;
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

	log4cxx::LoggerPtr m_logger;

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

#include "hxcomm/common/arqconnection.tcc"
