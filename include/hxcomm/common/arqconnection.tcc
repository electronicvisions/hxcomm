#include "hxcomm/common/signal.h"
#include "hate/math.h"

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
std::vector<sctrltp::packet> ARQConnection<ConnectionParameter>::SendQueue::move_to_packet_vector()
{
	size_t const num_packets =
	    hate::math::round_up_integer_division(m_subpackets.size(), MAX_PDUWORDS);
	size_t const last_packet_modulo = m_subpackets.size() % MAX_PDUWORDS;
	size_t const last_packet_len = last_packet_modulo ? last_packet_modulo : MAX_PDUWORDS;

	std::vector<sctrltp::packet> packets(num_packets);

	auto fill_packet = [&packets, this](size_t const packet_index, size_t const len) {
		size_t const base_subpacket_index = packet_index * MAX_PDUWORDS;
		for (size_t i = 0; i < len; ++i) {
			packets[packet_index][i] = m_subpackets[base_subpacket_index + i];
		}
		packets[packet_index].len = len;
		packets[packet_index].pid = pid;
	};

	for (size_t i = 0; i < num_packets - 1; ++i) {
		fill_packet(i, MAX_PDUWORDS);
	}
	fill_packet(num_packets - 1, last_packet_len);

	m_subpackets.clear();
	return packets;
}


template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection() :
    m_arq_stream(),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &ARQConnection<ConnectionParameter>::work_fill_receive_buffer, this),
    m_worker_decode_messages(&ARQConnection<ConnectionParameter>::work_decode_messages, this)
{}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::ARQConnection(ip_t const ip) :
    m_arq_stream(ip),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &ARQConnection<ConnectionParameter>::work_fill_receive_buffer, this),
    m_worker_decode_messages(&ARQConnection<ConnectionParameter>::work_decode_messages, this)
{}

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::~ARQConnection()
{
	m_run_receive = false;
	m_receive_buffer.notify();
	m_worker_fill_receive_buffer.join();
	m_worker_decode_messages.join();
}

template <typename ConnectionParameter>
template <class MessageType>
void ARQConnection<ConnectionParameter>::add(MessageType const& message)
{
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
	for (auto const packet : m_send_queue.move_to_packet_vector()) {
		m_arq_stream.send(packet, sctrltp::ARQStream::NOTHING);
	}
	m_arq_stream.flush();
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
			while (m_arq_stream.received_packet_available() &&
			       (packets_written < receive_buffer_size)) {
				m_arq_stream.receive(write_pointer->data[packets_written]);
				// discard packets with pid different from UT message pid
				if (write_pointer->data[packets_written].pid == pid) {
					packets_written++;
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

	constexpr size_t wait_period = 10000;
	while (!m_listener_halt.get()) {
		int ret = usleep(wait_period);
		if ((ret != 0) && errno != EINTR) {
			throw std::runtime_error("Error during usleep call.");
		}
	}
	m_listener_halt.reset();
}

} // namespace hxcomm
