namespace hxcomm {

template <typename UTMessageParameter>
LoopbackConnection<UTMessageParameter>::LoopbackConnection() :
    m_intermediate_queue_mutex(),
    m_intermediate_queue(),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_decoder(m_receive_queue),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &LoopbackConnection<UTMessageParameter>::work_fill_receive_buffer, this),
    m_worker_decode_messages(
        &LoopbackConnection<UTMessageParameter>::work_decode_messages, this)
{}

template <typename UTMessageParameter>
LoopbackConnection<UTMessageParameter>::~LoopbackConnection()
{
	m_run_receive = false;
	m_receive_buffer.notify();
	m_worker_fill_receive_buffer.join();
	m_worker_decode_messages.join();
}

template <typename UTMessageParameter>
template <class MessageType>
void LoopbackConnection<UTMessageParameter>::add(MessageType const& message)
{
	m_encoder(message);
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::add(
    std::vector<send_message_type> const& messages)
{
	m_encoder(messages);
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::commit()
{
	m_encoder.flush();
	size_t const size = m_send_queue.size();
	std::lock_guard<std::mutex> lock(m_intermediate_queue_mutex);
	for (size_t i = 0; i < size; ++i) {
		m_intermediate_queue.push_back(m_send_queue.front());
		m_send_queue.pop();
	}
}

template <typename UTMessageParameter>
typename LoopbackConnection<UTMessageParameter>::receive_message_type
LoopbackConnection<UTMessageParameter>::receive()
{
	receive_message_type message;
	if (__builtin_expect(!m_receive_queue.try_pop(message), false)) {
		throw std::runtime_error("No message available to receive.");
	}
	return message;
}

template <typename UTMessageParameter>
bool LoopbackConnection<UTMessageParameter>::try_receive(
    receive_message_type& message)
{
	return m_receive_queue.try_pop(message);
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::work_fill_receive_buffer()
{
	while (true) {
		auto const write_pointer = m_receive_buffer.start_write();
		if (!write_pointer) {
			m_receive_buffer.notify();
			return;
		}
		{
			std::lock_guard<std::mutex> lock(m_intermediate_queue_mutex);
			size_t const read_size = m_intermediate_queue.size();
			size_t const size = std::min(receive_buffer_size, read_size);
			std::copy(
			    m_intermediate_queue.cbegin(), m_intermediate_queue.cbegin() + size,
			    write_pointer->data.begin());
			for (size_t i = 0; i < size; ++i) {
				m_intermediate_queue.pop_front();
			}
			write_pointer->set_size(size);
		}
		m_receive_buffer.stop_write();
	}
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::work_decode_messages()
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

template <typename UTMessageParameter>
bool LoopbackConnection<UTMessageParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}

} // namespace hxcomm
