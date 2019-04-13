namespace hxcomm {

template <typename ConnectionParameter>
ARQConnection<ConnectionParameter>::SendQueue::SendQueue() : m_queue()
{
	sctrltp::packet pkg;
	pkg.len = 0;
	pkg.pid = pid;
	m_queue.push(pkg);
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::SendQueue::push(subpacket_type const& subpacket)
{
	if (m_queue.back().len < MAX_PDUWORDS) {
		m_queue.back().pdu[m_queue.back().len] = subpacket;
		m_queue.back().len++;
	} else {
		sctrltp::packet pkg;
		pkg.len = 1;
		pkg.pid = pid;
		pkg.pdu[0] = subpacket;
		m_queue.push(pkg);
	}
}

template <typename ConnectionParameter>
size_t ARQConnection<ConnectionParameter>::SendQueue::num_packets() const
{
	return m_queue.size();
}

template <typename ConnectionParameter>
sctrltp::packet& ARQConnection<ConnectionParameter>::SendQueue::front_packet()
{
	return m_queue.front();
}

template <typename ConnectionParameter>
sctrltp::packet const& ARQConnection<ConnectionParameter>::SendQueue::front_packet() const
{
	return m_queue.front();
}

template <typename ConnectionParameter>
void ARQConnection<ConnectionParameter>::SendQueue::pop_packet()
{
	m_queue.pop();
}


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
	size_t const num_packets = m_send_queue.num_packets();
	for (size_t i = 0; i < num_packets; ++i) {
		m_arq_stream.send(m_send_queue.front_packet(), sctrltp::ARQStream::NOTHING);
		m_send_queue.pop_packet();
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
	constexpr size_t wait_period = 10000;
	constexpr size_t timeout = 60 * 1000 * 1000 / wait_period;
	size_t counter = 0;
	while (!m_listener_halt.get()) {
		usleep(wait_period);
		if (counter++ > timeout) {
			throw std::runtime_error("Timeout while waiting for simulation to halt.");
		}
	}
	m_listener_halt.reset();
}

} // namespace hxcomm
