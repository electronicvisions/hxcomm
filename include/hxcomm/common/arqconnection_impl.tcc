#include "hate/math.h"
#include "hate/timer.h"
#include "hwdb4cpp/hwdb4cpp.h"
#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/signal.h"
#include <boost/asio/ip/address_v4.hpp>
#include <yaml-cpp/yaml.h>

#include <chrono>

namespace hxcomm {

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::SendQueue::SendQueue(arq_stream_type& arq_stream) :
    m_arq_stream(arq_stream), m_packet()
{
	m_packet.len = 0;
	m_packet.pid = pid;
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::SendQueue::push(subpacket_type const& subpacket)
{
	m_packet.pdu[m_packet.len] = subpacket;
	m_packet.len++;
	if (m_packet.len == sctrltp::ParametersFcpBss2Cube::MAX_PDUWORDS) {
		m_arq_stream.send(
		    m_packet, sctrltp::ARQStream<sctrltp::ParametersFcpBss2Cube>::Mode::NOTHING);
		m_packet.len = 0;
	}
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::SendQueue::flush()
{
	if (m_packet.len) {
		m_arq_stream.send(
		    m_packet, sctrltp::ARQStream<sctrltp::ParametersFcpBss2Cube>::Mode::FLUSH);
		m_packet.len = 0;
	} else {
		m_arq_stream.flush();
	}
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection() :
    m_registry(std::make_unique<Registry>(std::tuple{get_fpga_ip()})),
    m_arq_stream(std::make_unique<arq_stream_type>(std::get<0>(m_registry->m_parameters))),
    m_send_queue(*m_arq_stream),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection")),
    m_worker_receive()
{
	check_compatibility();
	m_worker_receive = std::thread(&ARQConnection<ConnectionParameter>::work_receive, this);
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection(ip_t const ip) :
    m_registry(std::make_unique<Registry>(std::tuple{ip})),
    m_arq_stream(std::make_unique<arq_stream_type>(std::get<0>(m_registry->m_parameters))),
    m_send_queue(*m_arq_stream),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection")),
    m_worker_receive()
{
	HXCOMM_LOG_TRACE(m_logger, "ARQConnection(): ARQ connection startup initiated.");
	check_compatibility();
	m_worker_receive = std::thread(&ARQConnection<ConnectionParameter>::work_receive, this);
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection(ARQConnection&& other) :
    m_registry(),
    m_arq_stream(),
    m_send_queue(other.m_send_queue),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue_mutex(),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection")),
    m_worker_receive()
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
	// move arq stream
	m_arq_stream = std::move(other.m_arq_stream);
	// move queues
	m_receive_queue.~receive_queue_type();
	new (&m_receive_queue) decltype(m_receive_queue)(std::move(other.m_receive_queue));
	// create decoder
	m_decoder.~decoder_type();
	new (&m_decoder) decltype(m_decoder)(other.m_decoder, m_receive_queue, m_listener_halt);
	// create and start threads
	m_worker_receive = std::thread(&ARQConnection<ConnectionParameter>::work_receive, this);
	HXCOMM_LOG_TRACE(m_logger, "ARQConnection(): ARQ connection startup initiated.");
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>& ARQConnection<ConnectionParameter>::operator=(
    ARQConnection&& other)
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
		// move arq stream
		m_arq_stream = std::move(other.m_arq_stream);
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
		m_worker_receive = std::thread(&ARQConnection<ConnectionParameter>::work_receive, this);
	}
	return *this;
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::~ARQConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~ARQConnection(): Stopping ARQ connection.");
	m_run_receive = false;
	if (m_worker_receive.joinable()) {
		m_worker_receive.join();
	}
}

template <typename ConnectionParameter>
std::mutex& ARQConnection<ConnectionParameter>::get_mutex()
{
	return m_mutex;
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::add(send_message_type const& message)
{
	hate::Timer timer;
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to moved-from ARQConnection.");
	}
	std::visit([this](auto const& m) { m_encoder(m); }, message);
	auto const duration = timer.get_ns();
	m_encode_duration.fetch_add(duration, std::memory_order_relaxed);
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::commit()
{
	hate::Timer timer;
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to moved-from ARQConnection.");
	}
	m_encoder.flush();
	m_send_queue.flush();
	auto const duration = timer.get_ns();
	m_commit_duration.fetch_add(duration, std::memory_order_relaxed);
	// Issue #3583 : Execution already starts upon sending
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
typename ARQConnection<ConnectionParameter>::receive_queue_type
ARQConnection<ConnectionParameter>::receive_all()
{
	receive_queue_type all;
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	std::swap(all, m_receive_queue);
	return all;
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::work_receive()
{
	HXCOMM_LOG_TRACE(m_logger, "work_receive() starting up..");
	sctrltp::packet<sctrltp::ParametersFcpBss2Cube> packet;
	while (m_run_receive) {
		while (m_arq_stream->received_packet_available() && m_run_receive) {
			HXCOMM_LOG_TRACE(m_logger, "Receiving new packet.");
			m_arq_stream->receive(packet);
			HXCOMM_LOG_TRACE(m_logger, "Received packet #" << packet.seq);
			if (packet.pid != pid) {
				std::stringstream ss;
				ss << "Unknown HostARQ packet ID received: " << packet.pid;
				throw std::runtime_error(ss.str());
			}
			HXCOMM_LOG_TRACE(m_logger, "Forwarding packet contents to decoder-coroutine..");
			hate::Timer timer;
			{
				std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
				m_decoder(packet.begin(), packet.end());
			}
			m_decode_duration.fetch_add(timer.get_ns(), std::memory_order_release);
			HXCOMM_LOG_TRACE(m_logger, "Forwarded packet contents to decoder-coroutine.");
		}
	}
	HXCOMM_LOG_TRACE(m_logger, "work_receive() terminating..");
}

template <typename ConnectionParameter>
bool ARQConnection<ConnectionParameter>::receive_empty() const
{
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::run_until_halt()
{
	using namespace std::literals::chrono_literals;

	hate::Timer timer;
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to moved-from ARQConnection.");
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
ConnectionTimeInfo ARQConnection<ConnectionParameter>::get_time_info() const
{
	return ConnectionTimeInfo{
	    std::chrono::nanoseconds(m_encode_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_decode_duration.load(std::memory_order_acquire)),
	    std::chrono::nanoseconds(m_commit_duration.load(std::memory_order_relaxed)),
	    std::chrono::nanoseconds(m_execution_duration.load(std::memory_order_relaxed))};
}

template <typename ConnectionParameter>
std::string ARQConnection<ConnectionParameter>::get_unique_identifier(
    std::optional<std::string> hwdb_path) const
{
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to empty instance.");
	}
	auto const ip = boost::asio::ip::make_address_v4(m_arq_stream->get_remote_ip());

	hwdb4cpp::database hwdb;
	hwdb.load(hwdb_path ? *hwdb_path : hwdb4cpp::database::get_default_path());
	std::optional<std::variant<hwdb4cpp::HXCubeSetupEntry, hwdb4cpp::JboaSetupEntry>> entry;
	size_t fcp = 0;
	auto const hxcube_ids = hwdb.get_hxcube_ids();
	for (auto const id : hxcube_ids) {
		auto const& local_entry = hwdb.get_hxcube_setup_entry(id);
		for (auto const& [f, e] : local_entry.fpgas) {
			if (e.ip == ip) {
				fcp = f;
				entry = local_entry;
				break;
			}
		}
	}
	auto const jboa_ids = hwdb.get_jboa_ids();
	for (auto const id : jboa_ids) {
		auto const& local_entry = hwdb.get_jboa_setup_entry(id);
		for (auto const& [f, e] : local_entry.fpgas) {
			if (e.ip == ip) {
				fcp = f;
				entry = local_entry;
				break;
			}
		}
	}
	if (!entry) {
		throw std::out_of_range("Setup not found in hwdb.");
	}
	return std::visit(
	    [fcp](auto const& entry) {
		    if (!entry.fpgas.at(fcp).wing) {
			    throw std::runtime_error("No chip present.");
		    }
		    return entry.get_unique_branch_identifier(
		        entry.fpgas.at(fcp).wing.value().handwritten_chip_serial);
	    },
	    *entry);
}

template <typename ConnectionParameter>
std::string ARQConnection<ConnectionParameter>::get_bitfile_info() const
{
	if (!m_arq_stream) {
		throw std::runtime_error("Unexpected access to empty instance.");
	}
	return m_arq_stream->get_response().bitfile_info;
}

template <typename ConnectionParameter>
std::string ARQConnection<ConnectionParameter>::get_remote_repo_state() const
{
	return "";
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::check_compatibility() const
{
	auto const yaml = m_arq_stream->get_response().bitfile_info;
	if (yaml.empty()) {
		throw std::runtime_error("Bitfile info empty");
	}
	YAML::Node info = YAML::Load(yaml);
	HXCOMM_LOG_DEBUG(m_logger, "check_compatibility(): bitfile info raw yaml:\n" << info);

	if (!(info["version"] && info["compatible_until"])) {
		throw std::runtime_error("Cannot find version info in bitfile info");
	}
	size_t const version = info["version"].as<size_t>();
	size_t const compat_until = info["compatible_until"].as<size_t>();
	if (version < oldest_supported_version) {
		throw std::runtime_error(
		    "Bitfile protocol version " + std::to_string(version) + " too old. Must be at least " +
		    std::to_string(oldest_supported_version));
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
