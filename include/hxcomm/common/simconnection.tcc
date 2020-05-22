#include "hxcomm/common/logger.h"

namespace hxcomm {

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(
    ip_t ip, port_t port, bool enable_terminate_on_destruction) :
    m_sim(std::make_unique<flange::SimulatorClient>(ip, port)),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer([&]() {
	    thread_local flange::SimulatorClient local_sim(ip, port);
	    work_fill_receive_buffer(local_sim);
    }),
    m_worker_decode_messages(&SimConnection<ConnectionParameter>::work_decode_messages, this),
    m_runnable_mutex(),
    m_terminate_on_destruction(enable_terminate_on_destruction),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(bool enable_terminate_on_destruction) :
    m_sim(std::make_unique<flange::SimulatorClient>()),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer([&]() {
	    thread_local flange::SimulatorClient local_sim;
	    work_fill_receive_buffer(local_sim);
    }),
    m_worker_decode_messages(&SimConnection<ConnectionParameter>::work_decode_messages, this),
    m_runnable_mutex(),
    m_terminate_on_destruction(enable_terminate_on_destruction),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(SimConnection&& other) :
    m_sim(),
    m_send_queue(),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(),
    m_worker_decode_messages(),
    m_runnable_mutex(),
    m_terminate_on_destruction(false),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	// shutdown other threads
	other.m_run_receive = false;
	other.m_receive_buffer.notify();
	other.m_worker_fill_receive_buffer.join();
	other.m_worker_decode_messages.join();
	// move simulator client
	m_sim = std::move(other.m_sim);
	// move queues
	m_send_queue = std::move(other.m_send_queue);
	m_receive_queue.~receive_queue_type();
	new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
	// create decoder
	m_decoder.~decoder_type();
	new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
	//
	m_worker_fill_receive_buffer = std::thread([&]() {
		thread_local flange::SimulatorClient local_sim;
		work_fill_receive_buffer(local_sim);
	});
	m_worker_decode_messages =
	    std::thread(&SimConnection<ConnectionParameter>::work_decode_messages, this);

	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::~SimConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~SimConnection(): Stopping Sim connection.");
	if (m_run_receive) {
		m_run_receive = false;
		m_receive_buffer.notify();
		m_worker_fill_receive_buffer.join();
		m_worker_decode_messages.join();
	}

	if (m_terminate_on_destruction) {
		std::unique_lock<std::mutex> lock(m_runnable_mutex);
		m_sim->set_runnable(true);
		m_sim->issue_terminate();
	}
}

template <typename ConnectionParameter>
std::mutex& SimConnection<ConnectionParameter>::get_mutex()
{
	return m_mutex;
}

template <typename ConnectionParameter>
template <class MessageType>
void SimConnection<ConnectionParameter>::add(MessageType const& message)
{
	HXCOMM_LOG_DEBUG(m_logger, "add(): Adding UT message to send queue: " << message);
	m_encoder(message);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::add(std::vector<send_message_type> const& messages)
{
	m_encoder(messages);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::commit()
{
	m_encoder.flush();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << m_send_queue.size() << " word(s).");
	while (!m_send_queue.empty()) {
		m_sim->send(m_send_queue.front());
		m_send_queue.pop();
	}
}

template <typename ConnectionParameter>
typename SimConnection<ConnectionParameter>::receive_message_type
SimConnection<ConnectionParameter>::receive()
{
	receive_message_type message;
	if (__builtin_expect(!m_receive_queue.try_pop(message), false)) {
		throw std::runtime_error("No message available to receive.");
	}
	return message;
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::try_receive(receive_message_type& message)
{
	return m_receive_queue.try_pop(message);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::work_fill_receive_buffer(
    flange::SimulatorClient& local_sim)
{
	while (true) {
		auto const write_pointer = m_receive_buffer.start_write();
		if (!write_pointer) {
			m_receive_buffer.notify();
			return;
		}
		{
			size_t words_written = 0;
			while (local_sim.receive_data_available() && (words_written < receive_buffer_size)) {
				write_pointer->data[words_written] = local_sim.receive();
				words_written++;
			}
			write_pointer->set_size(words_written);
		}
		m_receive_buffer.stop_write();
		if (!m_run_receive.load(std::memory_order_acquire))
			return;
	}
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::work_decode_messages()
{
	while (true) {
		auto const read_pointer = m_receive_buffer.start_read();
		if (!read_pointer) {
			m_receive_buffer.notify();
			return;
		}
		m_decoder(*read_pointer);
		m_receive_buffer.stop_read();
	}
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::run_until_halt()
{
	ResetHaltListener reset(m_listener_halt);
	ScopedSimulationRun run(*m_sim, m_runnable_mutex);

	constexpr size_t wait_period = 10000;
	while (!m_listener_halt.get()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
	}
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::set_enable_terminate_on_destruction(bool const value)
{
	m_terminate_on_destruction = value;
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::get_enable_terminate_on_destruction() const
{
	return m_terminate_on_destruction;
}

} // namespace hxcomm
