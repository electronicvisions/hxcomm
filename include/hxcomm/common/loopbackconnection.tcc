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
    m_worker_receive(&LoopbackConnection<UTMessageParameter>::work_receive, this)
{}

template <typename UTMessageParameter>
LoopbackConnection<UTMessageParameter>::~LoopbackConnection()
{
	m_run_receive = false;
	m_worker_receive.join();
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::add(send_message_type const& message)
{
	std::visit([this](auto const& m) { m_encoder(m); }, message);
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::add(std::vector<send_message_type> const& messages)
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
bool LoopbackConnection<UTMessageParameter>::try_receive(receive_message_type& message)
{
	return m_receive_queue.try_pop(message);
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::work_receive()
{
	while (m_run_receive) {
		std::lock_guard<std::mutex> lock(m_intermediate_queue_mutex);
		m_decoder(m_intermediate_queue);
		m_intermediate_queue.clear();
	}
}

template <typename UTMessageParameter>
bool LoopbackConnection<UTMessageParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}

} // namespace hxcomm
