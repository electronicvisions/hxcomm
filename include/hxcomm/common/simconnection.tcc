#include "hate/timer.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/sim_parameters.h"
#include <stdexcept>

namespace hxcomm {

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(
    ip_t ip, port_t port, bool enable_terminate_on_destruction) :
    m_registry(std::make_unique<Registry>(std::tuple{ip, port})),
    m_sim(std::make_unique<flange::SimulatorClient>(ip, port)),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_worker_receive([&]() {
	    thread_local flange::SimulatorClient local_sim(ip, port);
	    work_receive(local_sim);
    }),
    m_runnable_mutex(),
    m_terminate_on_destruction(enable_terminate_on_destruction),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	assert(m_sim);
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(bool enable_terminate_on_destruction) :
    m_registry(std::make_unique<Registry>(get_sim_parameters())),
    m_sim(std::make_unique<flange::SimulatorClient>(
        std::get<0>(m_registry->m_parameters), std::get<1>(m_registry->m_parameters))),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_worker_receive([&]() {
	    thread_local flange::SimulatorClient local_sim;
	    work_receive(local_sim);
    }),
    m_runnable_mutex(),
    m_terminate_on_destruction(enable_terminate_on_destruction),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	assert(m_sim);
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(SimConnection&& other) :
    m_registry(),
    m_sim(),
    m_send_queue(),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_worker_receive(),
    m_runnable_mutex(),
    m_terminate_on_destruction(false),
    m_logger(log4cxx::Logger::getLogger("hxcomm.SimConnection"))
{
	// shutdown other threads
	other.m_run_receive = false;
	other.m_worker_receive.join();
	m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
	m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
	m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
	m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
	// move registry
	m_registry = std::move(other.m_registry);
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
	m_worker_receive = std::thread([&]() {
		thread_local flange::SimulatorClient local_sim;
		work_receive(local_sim);
	});

	HXCOMM_LOG_TRACE(m_logger, "SimConnection(): Sim connection started.");

	// reset synplify wrapper to align behavior to ARQ FPGA reset of ARQConnection.
	if (!m_sim) {
		throw std::invalid_argument("Unexpected access to moved-from object.");
	}
	m_sim->issue_reset();
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>& SimConnection<ConnectionParameter>::operator=(
    SimConnection&& other)
{
	if (&other != this) {
		// shutdown own threads
		if (m_run_receive) {
			m_run_receive = false;
			m_worker_receive.join();
		}
		m_run_receive = static_cast<bool>(other.m_run_receive);
		// shutdown other threads
		if (other.m_run_receive) {
			other.m_run_receive = false;
			other.m_worker_receive.join();
		}
		m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
		m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
		m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
		m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
		// move registry
		m_registry = std::move(other.m_registry);
		// move simulation client
		m_sim = std::move(other.m_sim);
		// move queues
		m_send_queue = std::move(other.m_send_queue);
		m_receive_queue.~receive_queue_type();
		new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
		// create decoder
		m_decoder.~decoder_type();
		new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
		// create encoder
		m_encoder.~encoder_type();
		new (&m_encoder) encoder_type(other.m_encoder, m_send_queue);
		// create and start thread
		m_worker_receive = std::thread([&]() {
			thread_local flange::SimulatorClient local_sim;
			work_receive(local_sim);
		});
	}
	return *this;
}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::~SimConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~SimConnection(): Stopping Sim connection.");
	m_run_receive = false;
	if (m_worker_receive.joinable()) {
		m_worker_receive.join();
	}

	if (m_terminate_on_destruction && m_sim) {
		std::lock_guard<std::mutex> const lock(m_runnable_mutex);
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
void SimConnection<ConnectionParameter>::add(send_message_type const& message)
{
	hate::Timer timer;
	std::visit([this](auto const& m) { m_encoder(m); }, message);
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
template <typename InputIterator>
void SimConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	m_encoder(begin, end);
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::commit()
{
	hate::Timer timer;
	m_encoder.flush();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << m_send_queue.size() << " word(s).");
	if (!m_sim) {
		throw std::runtime_error("Unexpected access to moved-from object.");
	}
	while (!m_send_queue.empty()) {
		m_sim->send({m_send_queue.front()});
		m_send_queue.pop();
	}
	m_commit_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
typename SimConnection<ConnectionParameter>::receive_queue_type
SimConnection<ConnectionParameter>::receive_all()
{
	receive_queue_type all;
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	std::swap(all, m_receive_queue);
	return all;
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::work_receive(flange::SimulatorClient& local_sim)
{
	while (m_run_receive) {
		while (local_sim.receive_data_available() && m_run_receive) {
			hate::Timer timer;
			auto const words = local_sim.receive();
			m_decoder(words.begin(), words.end());
			m_decode_duration.fetch_add(timer.get_ns(), std::memory_order_release);
		}
	}
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::receive_empty() const
{
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::run_until_halt()
{
	ResetHaltListener reset(m_listener_halt);
	hate::Timer timer;
	if (!m_sim) {
		throw std::runtime_error("Unexpected access to moved-from object.");
	}
	ScopedSimulationRun run(*m_sim, m_runnable_mutex);

	constexpr size_t wait_period = 10000;
	while (!m_listener_halt.get()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
	}
	m_execution_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
ConnectionTimeInfo SimConnection<ConnectionParameter>::get_time_info() const
{
	return ConnectionTimeInfo{
	    std::chrono::nanoseconds(m_encode_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_decode_duration.load(std::memory_order_acquire)),
	    std::chrono::nanoseconds(m_commit_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_execution_duration.load(std::memory_order_relaxed))};
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

template <typename ConnectionParameter>
std::string SimConnection<ConnectionParameter>::get_unique_identifier(
    std::optional<std::string>) const
{
	// TODO: make unique
	return "simulation";
}

} // namespace hxcomm
