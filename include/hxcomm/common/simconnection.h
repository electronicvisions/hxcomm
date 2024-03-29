#pragma once
#include "flange/simulator_client.h"
#include "hate/visibility.h"
#include "hxcomm/common/connect_to_remote_parameter_defs.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_registry.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/decoder.h"
#include "hxcomm/common/encoder.h"
#include "hxcomm/common/listener_halt.h"
#include "hxcomm/common/signal.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/target.h"
#include "hxcomm/common/utmessage.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <stddef.h>
#include <stdint.h>
#include <thread>
#include <type_traits>
#include <unistd.h>
#include <vector>


namespace log4cxx {
class Logger;
typedef std::shared_ptr<Logger> LoggerPtr;
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
	HXCOMM_EXPOSE_MESSAGE_TYPES(ConnectionParameter)

	using init_parameters_type = typename std::tuple<ip_t, port_t>;

	static constexpr char name[] = "SimConnection";

	typedef std::vector<receive_message_type> receive_queue_type;

	/**
	 * Create and start connection to simulation server.
	 * @param ip IP-address of simulation server
	 * @param port Port of simulation server
	 * @param enable_terminate_on_destruction Whether or not to terminate the
	 * remote simulation upon destruction of the SimConnection.
	 */
	SimConnection(ip_t ip, port_t port, bool enable_terminate_on_destruction = false)
	    SYMBOL_VISIBLE;

	/**
	 * Create and start connection to simulation server.
	 * The RCF port is automatically extracted from the enviroment, the simulation server is
	 * expected to run on the same host.
	 * @param enable_terminate_on_destruction Whether or not to terminate the
	 * remote simulation upon destruction of the SimConnection.
	 * @throws std::runtime_error On no port to simulator found in environment
	 */
	SimConnection(bool enable_terminate_on_destruction = false) SYMBOL_VISIBLE;

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
	 * Move constructor.
	 */
	SimConnection(SimConnection&&) SYMBOL_VISIBLE;

	/**
	 * Assignment operator.
	 */
	SimConnection& operator=(SimConnection&&) SYMBOL_VISIBLE;

	/**
	 * Destruct simulation connection joining all receive threads.
	 */
	~SimConnection() SYMBOL_VISIBLE;

	/**
	 * Set enable value to terminate simulator on destruction of connection.
	 * @param value Boolean value
	 */
	void set_enable_terminate_on_destruction(bool const value) SYMBOL_VISIBLE;

	/**
	 * Get enable value to terminate simulator on destruction of connection.
	 * @return Boolean value
	 */
	bool get_enable_terminate_on_destruction() const SYMBOL_VISIBLE;

	constexpr static auto supported_targets = {Target::simulation};

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
	 * Returns "simulation" for SimConnection.
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
	 * Registry of open SimConnections.
	 */
	typedef ConnectionRegistry<SimConnection> Registry;
	std::unique_ptr<Registry> m_registry;

	friend class Stream<SimConnection>;

	/**
	 * Add a single UT message to the send queue.
	 * @param message Message to add
	 */
	void add(send_message_type const& message) SYMBOL_VISIBLE;

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
	 * Start simulation and wait until halt instruction is received from simulation.
	 * @throws std::runtime_error Simulation already running
	 */
	void run_until_halt() SYMBOL_VISIBLE;

	/**
	 * Get internal mutex to use for mutual exclusion.
	 * @return Mutable reference to mutex
	 */
	std::mutex& get_mutex() SYMBOL_VISIBLE;

	std::unique_ptr<flange::SimulatorClient> m_sim;

	typedef flange::SimulatorEvent::al_data_t::value_type subpacket_type;

	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Send::PhywordType>::value,
	    "flange al_data_t does not match send PhyWord type.");
	static_assert(
	    std::is_same<subpacket_type, typename ConnectionParameter::Receive::PhywordType>::value,
	    "flange al_data_t does not match receive PhyWord type.");

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

	void work_receive(flange::SimulatorClient& sim);
	std::thread m_worker_receive;

	std::mutex m_runnable_mutex;

	std::mutex m_mutex;

	typedef std::atomic<std::chrono::nanoseconds::rep> duration_type;
	duration_type m_encode_duration{};
	duration_type m_decode_duration{};
	duration_type m_commit_duration{};
	duration_type m_execution_duration{};

	bool m_terminate_on_destruction;
	log4cxx::LoggerPtr m_logger;

	struct ResetHaltListener
	{
		listener_halt_type& listener;

		ResetHaltListener(listener_halt_type& listener) : listener(listener) {}

		~ResetHaltListener()
		{
			listener.reset();
		}
	};

	struct ScopedSimulationRun
	{
		flange::SimulatorClient& client;
		std::unique_lock<std::mutex> lock;
		SignalOverrideIntTerm signal_override;

		ScopedSimulationRun(flange::SimulatorClient& client, std::mutex& mutex) :
		    client(client), lock(mutex), signal_override()
		{
			if (client.get_runnable()) {
				throw std::runtime_error("Trying to start already running simulation.");
			}

			// start simulation
			client.set_runnable(true);
			lock.unlock();
		}

		~ScopedSimulationRun()
		{
			lock.lock();
			client.set_runnable(false);
			lock.unlock();
		}
	};

	static_assert(
	    std::is_same_v<flange::SimulatorClient::port_t, port_t>, "Flange port type changed!");
	static_assert(std::is_same_v<flange::SimulatorClient::ip_t, ip_t>, "Flange ip type changed!");
};

} // namespace hxcomm

#include "hxcomm/common/simconnection.tcc"
