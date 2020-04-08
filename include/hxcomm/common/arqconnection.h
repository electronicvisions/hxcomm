#pragma once
#include <array>
#include <atomic>
#include <memory>
#include <queue>
#include <thread>
#include <tuple>

#include <tbb/concurrent_queue.h>

#include "sctrltp/ARQFrame.h"
#include "sctrltp/ARQStream.h"

#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/double_buffer.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/listener_halt.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/utmessage.h"

namespace log4cxx {
class Logger;
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
	using message_types = MessageTypes<ConnectionParameter>;

	using receive_message_type = typename message_types::receive_type;
	using send_message_type = typename message_types::send_type;
	using send_halt_message_type = typename message_types::send_halt_type;

	using init_parameters_type = typename std::tuple<ip_t>;

	/**
	 * Create connection to FPGA with IP address found in environment.
	 * @throws std::runtime_error On no or more than one FPGA IP address available in environment
	 */
	ARQConnection();

	/**
	 * Create connection to FPGA.
	 * @param ip IP-address of FPGA
	 */
	ARQConnection(ip_t ip);

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
	ARQConnection(ARQConnection&& other);

	/**
	 * Assignment operator.
	 */
	ARQConnection& operator=(ARQConnection&& other) = default;

	/**
	 * Destruct connection to FPGA joining all receive threads.
	 */
	~ARQConnection();

private:
	friend Stream<ARQConnection>;
	/**
	 * Add a single UT message to the send queue.
	 * @tparam MessageType Type of message to add
	 * @param message Message to add
	 */
	template <class MessageType>
	void add(MessageType const& message);

	/**
	 * Add multiple UT messages to the send queue.
	 * @param messages Messages to add
	 */
	void add(std::vector<send_message_type> const& messages);

	/**
	 * Send messages in send queue.
	 */
	void commit();

	/**
	 * Receive a single UT message.
	 * @throws std::runtime_error On empty message queue
	 * @return Received message
	 */
	receive_message_type receive();

	/**
	 * Try to receive a single UT message.
	 * @param message Message to receive to
	 * @return Boolean value whether receive was successful
	 */
	bool try_receive(receive_message_type& message);

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const;

	/**
	 * Start execution and wait until halt instruction is received from FPGA.
	 */
	void run_until_halt();

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
		SendQueue();

		void push(subpacket_type const& subpacket);

		std::vector<sctrltp::packet<sctrltp::ParametersFcpBss2Cube>> move_to_packet_vector();

	private:
		std::vector<subpacket_type> m_subpackets;
	};

	typedef SendQueue send_queue_type;
	send_queue_type m_send_queue;

	Encoder<typename ConnectionParameter::Send, send_queue_type> m_encoder;

	typedef tbb::concurrent_queue<receive_message_type> receive_queue_type;
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

	constexpr static size_t receive_buffer_size = 100;
	DoubleBuffer<Packet<sctrltp::packet<sctrltp::ParametersFcpBss2Cube>, receive_buffer_size>>
	    m_receive_buffer;

	void work_fill_receive_buffer();
	std::thread m_worker_fill_receive_buffer;

	void work_decode_messages();
	std::thread m_worker_decode_messages;

	log4cxx::Logger* m_logger;
};

} // namespace hxcomm

#include "hxcomm/common/arqconnection.tcc"
