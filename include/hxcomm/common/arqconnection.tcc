#include <boost/asio/ip/address_v4.hpp>
#include "hate/math.h"
#include "hate/timer.h"
#include "hwdb4cpp/hwdb4cpp.h"
#include "hxcomm/common/fpga_ip_list.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/signal.h"

namespace hxcomm {

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::SendQueue::SendQueue() : m_subpackets()
{}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::SendQueue::push(subpacket_type const& subpacket)
{
	m_subpackets.push_back(subpacket);
}

template <typename ConnectionParameter>
std::vector<sctrltp::packet<sctrltp::ParametersFcpBss2Cube>>
ARQConnection<ConnectionParameter>::SendQueue::move_to_packet_vector()
{
	size_t const num_packets = hate::math::round_up_integer_division(
	    m_subpackets.size(), sctrltp::Parameters<>::MAX_PDUWORDS);
	size_t const last_packet_modulo = m_subpackets.size() % sctrltp::Parameters<>::MAX_PDUWORDS;
	size_t const last_packet_len =
	    last_packet_modulo ? last_packet_modulo : sctrltp::Parameters<>::MAX_PDUWORDS;

	std::vector<sctrltp::packet<sctrltp::ParametersFcpBss2Cube>> packets(num_packets);

	auto fill_packet = [&packets, this](size_t const packet_index, size_t const len) {
		size_t const base_subpacket_index = packet_index * sctrltp::Parameters<>::MAX_PDUWORDS;
		for (size_t i = 0; i < len; ++i) {
			packets[packet_index].pdu[i] = m_subpackets[base_subpacket_index + i];
		}
		packets[packet_index].len = len;
		packets[packet_index].pid = pid;
	};

	for (size_t i = 0; i < num_packets - 1; ++i) {
		fill_packet(i, sctrltp::Parameters<>::MAX_PDUWORDS);
	}
	fill_packet(num_packets - 1, last_packet_len);

	m_subpackets.clear();
	return packets;
}


template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection() :
    m_arq_stream(std::make_unique<arq_stream_type>(get_fpga_ip())),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_worker_receive(&ARQConnection<ConnectionParameter>::work_receive, this),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection"))
{}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection(ip_t const ip) :
    m_arq_stream(std::make_unique<arq_stream_type>(ip)),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_worker_receive(&ARQConnection<ConnectionParameter>::work_receive, this),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection"))
{
	HXCOMM_LOG_TRACE(m_logger, "ARQConnection(): ARQ connection startup initiated.");
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection(ARQConnection&& other) :
    m_arq_stream(),
    m_send_queue(),
    m_encoder(other.m_encoder, m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt), // temporary
    m_run_receive(true),
    m_worker_receive(),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection"))
{
	// shutdown other threads
	other.m_run_receive = false;
	other.m_worker_receive.join();
	m_encode_duration = other.m_encode_duration.load(std::memory_order_relaxed);
	m_decode_duration = other.m_decode_duration.load(std::memory_order_relaxed);
	m_commit_duration = other.m_commit_duration.load(std::memory_order_relaxed);
	m_execution_duration = other.m_execution_duration.load(std::memory_order_relaxed);
	// move arq stream
	m_arq_stream = std::move(other.m_arq_stream);
	// move queues
	m_send_queue = std::move(other.m_send_queue);
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
ARQConnection<ConnectionParameter>::~ARQConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~ARQConnection(): Stopping ARQ connection.");
	if (m_run_receive) {
		m_run_receive = false;
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
	HXCOMM_LOG_DEBUG(m_logger, "add(): Adding UT message to send queue: " << message);
	boost::apply_visitor([this](auto const& m) { m_encoder(m); }, message);
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::add(std::vector<send_message_type> const& messages)
{
	hate::Timer timer;
	m_encoder(messages);
	m_encode_duration.fetch_add(timer.get_ns(), std::memory_order_relaxed);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::commit()
{
	hate::Timer timer;
	m_encoder.flush();
	auto const packets = m_send_queue.move_to_packet_vector();
	[[maybe_unused]] size_t const num_packets = packets.size();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << num_packets << " ARQ packet(s).");
	for (auto const& packet : packets) {
		m_arq_stream->send(packet, arq_stream_type::NOTHING);
	}
	m_arq_stream->flush();
	auto const duration = timer.get_ns();
	m_commit_duration.fetch_add(duration, std::memory_order_relaxed);
	// Issue #3583 : Execution already starts upon sending
	m_execution_duration.fetch_add(duration, std::memory_order_relaxed);
}

template <typename ConnectionParameter>
typename ARQConnection<ConnectionParameter>::receive_message_type
ARQConnection<ConnectionParameter>::receive()
{
	receive_message_type message;
	if (__builtin_expect(!m_receive_queue.try_pop(message), false)) {
		throw std::runtime_error("No message available to receive.");
	}
	return message;
}

template <typename ConnectionParameter>
bool ARQConnection<ConnectionParameter>::try_receive(receive_message_type& message)
{
	return m_receive_queue.try_pop(message);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::work_receive()
{
	sctrltp::packet<sctrltp::ParametersFcpBss2Cube> packet;
	while (m_run_receive) {
		while (m_arq_stream->received_packet_available() && m_run_receive) {
			m_arq_stream->receive(packet);
			if (packet.pid != pid) {
				std::stringstream ss;
				ss << "Unknown HostARQ packet ID received: " << packet.pid;
				throw std::runtime_error(ss.str());
			}
			hate::Timer timer;
			for (size_t i = 0; i < packet.len; ++i) {
				m_decoder(packet.pdu[i]);
			}
			m_decode_duration.fetch_add(timer.get_ns(), std::memory_order_release);
		}
	}
}

template <typename ConnectionParameter>
bool ARQConnection<ConnectionParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::run_until_halt()
{
	hate::Timer timer;
	SignalOverrideIntTerm signal_override;

	size_t wait_period = 1;
	constexpr size_t max_wait_period = 10000;
	while (!m_listener_halt.get()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
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
	auto const hxcube_ids = hwdb.get_hxcube_ids();
	hwdb4cpp::HXCubeSetupEntry entry;
	size_t fcp;
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
	if (!entry.fpgas.at(fcp).wing) {
		throw std::runtime_error("No chip present.");
	}
	return entry.get_unique_branch_identifier(
	    entry.fpgas.at(fcp).wing.value().handwritten_chip_serial);
}

} // namespace hxcomm
