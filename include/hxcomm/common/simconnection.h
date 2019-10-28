#pragma once
#include <atomic>
#include <queue>
#include <thread>

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include <tbb/concurrent_queue.h>

#include "flange/simulator_client.h"

#include "hxcomm/common/decoder.h"
#include "hxcomm/common/double_buffer.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/listener_halt.h"
#include "hxcomm/common/utmessage.h"

namespace log4cxx {
class Logger;
} // namespace log4cxx

namespace hxcomm {

/**
 * Simulation connection class.
 * Establish and hold Simulation connection to FPGA.
 * Provide convenience functions for sending and receiving UT messages.
 * @tparam ConnectionParameter UT message parameter for connection
 */
template <typename ConnectionParameter>
class SimConnection
{
public:
	typedef flange::SimulatorClient::port_t port_t;
	typedef flange::SimulatorClient::ip_t ip_t;
	typedef SimConnection connection_t;

	typedef typename ToUTMessageVariant<
	    ConnectionParameter::Send::HeaderAlignment,
	    typename ConnectionParameter::Send::SubwordType,
	    typename ConnectionParameter::Send::PhywordType,
	    typename ConnectionParameter::Send::Dictionary>::type send_message_type;

	typedef typename ToUTMessageVariant<
	    ConnectionParameter::Receive::HeaderAlignment,
	    typename ConnectionParameter::Receive::SubwordType,
	    typename ConnectionParameter::Receive::PhywordType,
	    typename ConnectionParameter::Receive::Dictionary>::type receive_message_type;

	/**
	 * Create and start connection to simulation server.
	 * @param ip IP-address of simulation server
	 * @param port Port of simulation server
	 */
	SimConnection(ip_t ip, port_t port);

	/**
	 * Copy constructor (deleted because no two instances with the same simulator allocation can
	 * coexist).
	 */
	SimConnection(SimConnection const&) = delete;

	/**
	 * Assignment operator (deleted because no two instances with the same simulator allocation can
	 * coexist).
	 */
	SimConnection& operator=(SimConnection const&) = delete;

	/**
	 * Destruct simulation connection joining all receive threads.
	 */
	~SimConnection();

	/**
	 * Add a single UT message to the send queue.
	 * @tparam MessageType Type of message to add
	 * @param message Message to add
	 */
	template <typename MessageType>
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
	 * @return Boolean value whether receive was successfule
	 */
	bool try_receive(receive_message_type& message);

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const;

	/**
	 * Start simulation and wait until simulation progressed a specified number of clock count.
	 * @param clock Clock cycle duration after when to pause simulation
	 * @throws std::runtime_error Simulation already running
	 */
	void run_for(flange::SimulatorEvent::clk_t clock);

	/**
	 * Start simulation and wait until halt instruction is received from simulation.
	 * @throws std::runtime_error Simulation already running
	 */
	void run_until_halt();

	/**
	 * Set enable value to terminate simulator on destruction of connection.
	 * @param value Boolean value
	 */
	void set_enable_terminate_on_destruction(bool value);

	/**
	 * Get enable value to terminate simulator on destruction of connection.
	 * @return Boolean value
	 */
	bool get_enable_terminate_on_destruction() const;

private:
	flange::SimulatorClient m_sim;

	typedef flange::SimulatorEvent::al_data_t subpacket_type;

	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Send::PhywordType>::value,
	    "flange al_data_t does not match send PhyWord type.");
	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Receive::PhywordType>::value,
	    "flange al_data_t does not match receive PhyWord type.");

	typedef std::queue<subpacket_type> send_queue_type;
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

	Decoder<
	    typename ConnectionParameter::Receive,
	    receive_queue_type,
	    listener_halt_type>
	    m_decoder;

	std::atomic<bool> m_run_receive;

	constexpr static size_t receive_buffer_size = 100000;
	DoubleBuffer<Packet<subpacket_type, receive_buffer_size>> m_receive_buffer;

	void work_fill_receive_buffer(ip_t ip, port_t port);
	std::thread m_worker_fill_receive_buffer;

	void work_decode_messages();
	std::thread m_worker_decode_messages;

	std::mutex m_runnable_mutex;

	bool m_terminate_on_destruction;
	log4cxx::Logger* m_logger;
};

} // namespace hxcomm

#include "hxcomm/common/simconnection.tcc"
