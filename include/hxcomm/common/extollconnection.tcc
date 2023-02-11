#include "hate/math.h"
#include "hate/timer.h"
#include "hwdb4cpp/hwdb4cpp.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/signal.h"
#include "nhtl-extoll/buffer.h"
#include "nhtl-extoll/configure_fpga.h"
#include <sched.h>
#include <yaml-cpp/yaml.h>

#include <chrono>
#include <cstdint>
#include <cstring>

namespace hxcomm {

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>::SendQueue::SendQueue(nhtl_extoll::Endpoint& connection) :
	m_connection(connection), m_packets(), m_cum_packets() {}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::SendQueue::push(subpacket_type const& subpacket)
{
	m_connection.buffer.write_send(m_packets, subpacket);
	m_packets++;
	m_cum_packets++;
	if (m_packets == m_connection.buffer.send_buffer_size_qw()) {
		m_connection.rma_send(m_packets);
		m_packets = 0;
	}
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::SendQueue::flush()
{
	if (m_packets) {
		m_connection.rma_send(m_packets);
		m_packets = 0;
	}
	m_cum_packets = 0;
}

template <typename ConnectionParameter>
size_t ExtollConnection<ConnectionParameter>::SendQueue::cum_size()
{
	return m_cum_packets;
}

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>::ExtollConnection() :
    m_registry(std::make_unique<Registry>(std::tuple{nhtl_extoll::get_fpga_node_id()})),
    m_connection(std::make_unique<nhtl_extoll::Endpoint>(std::get<0>(m_registry->m_parameters))),
    m_send_queue(*m_connection),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ExtollConnection")),
    m_worker_receive(&ExtollConnection<ConnectionParameter>::work_receive, this)
{
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection(): establishing Extoll connection to node "
	                          + std::to_string(nhtl_extoll::get_fpga_node_id()) + ".");
	check_compatibility();
	nhtl_extoll::configure_fpga(*m_connection);
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection(): Extoll connection established to node "
	                          + std::to_string(nhtl_extoll::get_fpga_node_id()) + ".");
}

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>::ExtollConnection(RMA2_Nodeid node_id) :
    m_registry(std::make_unique<Registry>(std::tuple{node_id})),
    m_connection(std::make_unique<nhtl_extoll::Endpoint>(std::get<0>(m_registry->m_parameters))),
    m_send_queue(*m_connection),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ExtollConnection")),
    m_worker_receive(&ExtollConnection<ConnectionParameter>::work_receive, this)
{
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection(): establishing Extoll connection to node with id "
	                          + std::to_string(node_id) + ".");
	check_compatibility();
	nhtl_extoll::configure_fpga(*m_connection);
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection(): Extoll connection established to node with id "
	                          + std::to_string(node_id) + ".");
}

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>::ExtollConnection(ExtollConnection&& other) :
    m_registry(),
	m_connection(),
    m_send_queue(other.m_send_queue),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ExtollConnection")),
    m_worker_receive()
{
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection() [move]: start moving conn to "
	                          + std::to_string(std::get<0>(other.m_registry->m_parameters)) + ".");
	// shutdown other threads
	other.m_run_receive = false;
	other.m_worker_receive.join();
	m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
	m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
	m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
	m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
	// move registry
	m_registry = std::move(other.m_registry);
	// move nhtl-extoll endpoint
	m_connection = std::move(other.m_connection);
	// move queues
	m_receive_queue.~receive_queue_type();
	new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
	// create decoder
	m_decoder.~decoder_type();
	new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
	// create and start threads
	m_worker_receive = std::thread(&ExtollConnection<ConnectionParameter>::work_receive, this);
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection() [move]: finished moving conn to "
	                          + std::to_string(std::get<0>(other.m_registry->m_parameters)) + ".");
}

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>& ExtollConnection<ConnectionParameter>::operator=(
    ExtollConnection&& other)
{
	if (&other != this) {
		HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection::operator=() [move]: start.");
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
		// move nhtl-extoll endpoint
		m_connection = std::move(other.m_connection);
		// move queues
		m_send_queue.~send_queue_type();
		new (&m_send_queue) send_queue_type(other.m_send_queue);
		m_receive_queue.~receive_queue_type();
		new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
		// create decoder
		m_decoder.~decoder_type();
		new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
		// create encoder
		m_encoder.~encoder_type();
		new (&m_encoder) encoder_type(other.m_encoder, m_send_queue);
		// create and start thread
		m_worker_receive = std::thread(&ExtollConnection<ConnectionParameter>::work_receive, this);
		HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection::operator=() [move]: Extoll connection startup initiated.");
	}
	return *this;
}

template <typename ConnectionParameter>
ExtollConnection<ConnectionParameter>::~ExtollConnection()
{
	HXCOMM_LOG_DEBUG(m_logger, "~ExtollConnection(): Stopping Extoll connection.");
	m_run_receive = false;
	if (m_worker_receive.joinable()) {
		m_worker_receive.join();
	}
	HXCOMM_LOG_DEBUG(m_logger, "~ExtollConnection(): Extoll connection stopped.");
}

template <typename ConnectionParameter>
std::mutex& ExtollConnection<ConnectionParameter>::get_mutex()
{
	return m_mutex;
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::add(send_message_type const& message)
{
	hate::Timer timer;
	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}
	std::visit([this](auto const& m) { m_encoder(m); }, message);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
template <typename InputIterator>
void ExtollConnection<ConnectionParameter>::add(InputIterator const& begin, InputIterator const& end)
{
	hate::Timer timer;
	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}
	m_encoder(begin, end);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::commit()
{
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection::run_until_halt() on node_id " + std::to_string(m_connection->get_node()));

	hate::Timer timer;
	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}
	m_encoder.flush();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << m_send_queue.cum_size() << " word(s).");
	m_send_queue.flush();
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
typename ExtollConnection<ConnectionParameter>::receive_queue_type
ExtollConnection<ConnectionParameter>::receive_all()
{
	receive_queue_type all;
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	std::swap(all, m_receive_queue);
	return all;
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::work_receive()
{
	HXCOMM_LOG_DEBUG(m_logger, "Pinning receiving thread to same CPU as polling thread.");
	sched_setaffinity(0, sizeof(cpu_set_t), &(m_connection->poller.cpu));

	using namespace std::literals::chrono_literals;
	auto wait_period = 1us;
	constexpr std::chrono::microseconds max_wait_period = 10ms;

	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}

	while (m_run_receive) {
		auto const& data = m_connection->trace_ring_buffer.receive();

		if (data.size() == 0) {
			std::this_thread::sleep_for(wait_period);
			wait_period = std::min(wait_period * 2, max_wait_period);
			continue;
		}
		wait_period = 1us;

		HXCOMM_LOG_TRACE(m_logger, "Forwarding packet contents to decoder-coroutine..");
		hate::Timer timer;
		{
			std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
			m_decoder(data.begin(), data.end());
		}
		m_decode_duration.fetch_add(timer.get_ns(), std::memory_order_release);
		HXCOMM_LOG_TRACE(m_logger, "Forwarded packet contents to decoder-coroutine.");
	}
}

template <typename ConnectionParameter>
bool ExtollConnection<ConnectionParameter>::receive_empty() const
{
    std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::run_until_halt()
{
	HXCOMM_LOG_DEBUG(m_logger, "ExtollConnection::run_until_halt() on node_id " + std::to_string(m_connection->get_node()));

	using namespace std::literals::chrono_literals;
	hate::Timer timer;
	if (!m_connection) {
		throw std::runtime_error("Unexpected access to moved-from ExtollConnection.");
	}
	SignalOverrideIntTerm signal_override;
	auto wait_period = 1us;
	constexpr std::chrono::microseconds max_wait_period = 10ms;
	while (!m_listener_halt.get()) {
		std::this_thread::sleep_for(wait_period);
		wait_period = std::min(wait_period * 2, max_wait_period);
	}
	m_listener_halt.reset();
	m_execution_duration += timer.get_ns();
}

template <typename ConnectionParameter>
ConnectionTimeInfo ExtollConnection<ConnectionParameter>::get_time_info() const
{
	return ConnectionTimeInfo{
	    std::chrono::nanoseconds(m_encode_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_decode_duration.load(std::memory_order_acquire)),
	    std::chrono::nanoseconds(m_commit_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_execution_duration.load(std::memory_order_relaxed))};
}

template <typename ConnectionParameter>
std::string ExtollConnection<ConnectionParameter>::get_unique_identifier(
    std::optional<std::string> hwdb_path) const
{
    if (!m_connection) {
        throw std::runtime_error("Unexpected access to empty instance.");
    }
    RMA2_Nodeid const node_id = m_connection->get_node();

    hwdb4cpp::database hwdb;
    hwdb.load(hwdb_path ? *hwdb_path : hwdb4cpp::database::get_default_path());
    auto const hxcube_ids = hwdb.get_hxcube_ids();
    hwdb4cpp::HXCubeSetupEntry entry;
    size_t fcp;
    for (auto const id : hxcube_ids) {
        auto const& local_entry = hwdb.get_hxcube_setup_entry(id);
        for (auto const& [f, e] : local_entry.fpgas) {
            if (!e.extoll_node_id) {
                continue;
            }
            if (e.extoll_node_id == node_id) {
                fcp = f;
                entry = local_entry;
                break;
            }
        }
    }
    if (!entry.fpgas.at(fcp).wing) {
        throw std::runtime_error("No chip present.");
    }
    return entry.get_unique_branch_identifier(
        entry.fpgas.at(fcp).wing.value().handwritten_chip_serial);
}

template <typename ConnectionParameter>
std::string ExtollConnection<ConnectionParameter>::get_bitfile_info() const
{
    if (!m_connection) {
        throw std::runtime_error("Unexpected access to empty instance.");
    }
	std::vector<uint64_t> bitfile_info_binary;
	for (int i = 0; i < 512; i++) {
		bitfile_info_binary.push_back(m_connection->rra_read(0x0 + i * 0x8));
	}
	std::vector<char> info_c_str;
	for (size_t i = 0; i < bitfile_info_binary.size(); i++) {
		for (size_t j = 0; j < sizeof(uint64_t); j++) {
			char const my_char = (be64toh(bitfile_info_binary[i]) >> (j * sizeof(uint64_t))) & 0xff;
			info_c_str.push_back(my_char);
		}
	}
	return std::string(info_c_str.data());
}

template <typename ConnectionParameter>
std::string ExtollConnection<ConnectionParameter>::get_remote_repo_state() const
{
	return "";
}

template <typename ConnectionParameter>
void ExtollConnection<ConnectionParameter>::check_compatibility() const
{
	std::string const yaml = get_bitfile_info();
	if (yaml.empty()) {
		throw std::runtime_error("Bitfile info empty");
	}
	YAML::Node info = YAML::Load(yaml);
	HXCOMM_LOG_DEBUG(m_logger, "check_compatibility(): bitfile info raw yaml:\n" << info);

	if(!(info["version"] && info["compatible_until"])) {
		throw std::runtime_error("Cannot find version info in bitfile info");
	}
	size_t const version = info["version"].as<size_t>();
	size_t const compat_until = info["compatible_until"].as<size_t>();
	if (version < oldest_supported_version) {
		throw std::runtime_error(
		    "Bitfile protocol version " + std::to_string(version) +
		    " too old. Must be at least " + std::to_string(oldest_supported_version));
	}
	if (compat_until > newest_supported_compatible_until) {
		throw std::runtime_error(
		    "Software too old. Bitfile needs at least " + std::to_string(compat_until) +
		    ". Software only supports " + std::to_string(newest_supported_compatible_until));
	}
	HXCOMM_LOG_INFO(
	    m_logger, "check_compatibility():\n\tFPGA:\tversion: "
	                  << version << "\t compat_until: " << compat_until
	                  << "\tSW:\tversion: " << oldest_supported_version
	                  << "\t compat_until: " << newest_supported_compatible_until);
}

} // namespace hxcomm
