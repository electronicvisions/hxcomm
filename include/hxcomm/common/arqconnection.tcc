#include "hate/math.h"
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
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &ARQConnection<ConnectionParameter>::work_fill_receive_buffer, this),
    m_worker_decode_messages(&ARQConnection<ConnectionParameter>::work_decode_messages, this),
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
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &ARQConnection<ConnectionParameter>::work_fill_receive_buffer, this),
    m_worker_decode_messages(&ARQConnection<ConnectionParameter>::work_decode_messages, this),
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
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(),
    m_worker_decode_messages(),
    m_logger(log4cxx::Logger::getLogger("hxcomm.ARQConnection"))
{
	// shutdown other threads
	other.m_run_receive = false;
	other.m_receive_buffer.notify();
	other.m_worker_fill_receive_buffer.join();
	other.m_worker_decode_messages.join();
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
	m_worker_fill_receive_buffer =
	    std::thread(&ARQConnection<ConnectionParameter>::work_fill_receive_buffer, this);
	m_worker_decode_messages =
	    std::thread(&ARQConnection<ConnectionParameter>::work_decode_messages, this);
	HXCOMM_LOG_TRACE(m_logger, "ARQConnection(): ARQ connection startup initiated.");
}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::~ARQConnection()
{
	HXCOMM_LOG_TRACE(m_logger, "~ARQConnection(): Stopping ARQ connection.");
	if (m_run_receive) {
		m_run_receive = false;
		m_receive_buffer.notify();
		m_worker_fill_receive_buffer.join();
		m_worker_decode_messages.join();
	}
}

template <typename ConnectionParameter>
template <class MessageType>
void ARQConnection<ConnectionParameter>::add(MessageType const& message)
{
	HXCOMM_LOG_DEBUG(m_logger, "add(): Adding UT message to send queue: " << message);
	m_encoder(message);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::add(std::vector<send_message_type> const& messages)
{
	m_encoder(messages);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::commit()
{
	m_encoder.flush();
	auto const packets = m_send_queue.move_to_packet_vector();
	[[maybe_unused]] size_t const num_packets = packets.size();
	HXCOMM_LOG_DEBUG(m_logger, "commit(): Commiting " << num_packets << " ARQ packet(s).");
	for (auto const packet : packets) {
		m_arq_stream->send(packet, arq_stream_type::NOTHING);
	}
	m_arq_stream->flush();
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
void ARQConnection<ConnectionParameter>::work_fill_receive_buffer()
{
	while (true) {
		auto const write_pointer = m_receive_buffer.start_write();
		if (!write_pointer) {
			m_receive_buffer.notify();
			return;
		}
		{
			size_t packets_written = 0;
			while (m_arq_stream->received_packet_available() &&
			       (packets_written < receive_buffer_size)) {
				m_arq_stream->receive(write_pointer->data[packets_written]);
				if (write_pointer->data[packets_written].pid == pid) {
					packets_written++;
				} else {
					std::stringstream ss;
					ss << "Unknown HostARQ packet ID received: "
					   << write_pointer->data[packets_written].pid;
					throw std::runtime_error(ss.str());
				}
			}
			write_pointer->set_size(packets_written);
		}
		m_receive_buffer.stop_write();
	}
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::work_decode_messages()
{
	while (true) {
		auto const read_pointer = m_receive_buffer.start_read();
		if (!read_pointer) {
			m_receive_buffer.notify();
			return;
		}
		for (auto it = read_pointer->cbegin(); it < read_pointer->cend(); ++it) {
			for (size_t i = 0; i < it->len; ++i) {
				m_decoder(it->pdu[i]);
			}
		}
		m_receive_buffer.stop_read();
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
}

} // namespace hxcomm
