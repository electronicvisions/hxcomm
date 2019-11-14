#include "hxcomm/common/logger.h"
#include "hxcomm/common/signal.h"

namespace hxcomm {

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(ip_t ip, port_t port) :
    m_sim(ip, port),
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
    m_terminate_on_destruction(false),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	m_sim.issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection() :
    m_sim(),
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
    m_terminate_on_destruction(false),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	m_sim.issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::~SimConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~SimConnection(): Stopping Sim connection.");
	m_run_receive = false;
	m_receive_buffer.notify();
	m_worker_fill_receive_buffer.join();
	m_worker_decode_messages.join();

	if (m_terminate_on_destruction) {
		std::unique_lock<std::mutex> lock(m_runnable_mutex);
		m_sim.set_runnable(true);
		m_sim.issue_terminate();
	}
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
	HXCOMM_LOG_INFO(m_logger, "commit(): Commiting " << m_send_queue.size() << " word(s).");
	while (!m_send_queue.empty()) {
		m_sim.send(m_send_queue.front());
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
void SimConnection<ConnectionParameter>::run_for(flange::SimulatorEvent::clk_t clock)
{
	std::unique_lock<std::mutex> lock(m_runnable_mutex);
	if (m_sim.get_runnable()) {
		throw std::runtime_error("Trying to start already running simulation.");
	}

	SignalOverrideIntTerm signal_override;

	flange::SimulatorEvent::clk_t current_time = m_sim.get_current_time();

	// start simulation
	m_sim.set_runnable(true);
	lock.unlock();

	constexpr size_t wait_period = 10000;
	while (m_sim.get_runnable()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
		if (m_sim.get_current_time() >= current_time + clock) {
			break;
		}
	}

	lock.lock();
	m_sim.set_runnable(false);
	lock.unlock();
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::run_until_halt()
{
	std::unique_lock<std::mutex> lock(m_runnable_mutex);
	if (m_sim.get_runnable()) {
		throw std::runtime_error("Trying to start already running simulation.");
	}

	SignalOverrideIntTerm signal_override;

	m_listener_halt.reset();

	// start simulation
	m_sim.set_runnable(true);
	lock.unlock();

	constexpr size_t wait_period = 10000;
	while (!m_listener_halt.get()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
	}

	lock.lock();
	m_sim.set_runnable(false);
	lock.unlock();
	m_listener_halt.reset();
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
