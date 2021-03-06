namespace hxcomm {

template <typename UTMessageParameter>
LoopbackConnection<UTMessageParameter>::LoopbackConnection() :
    m_intermediate_queue_mutex(),
    m_intermediate_queue(),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue_mutex(),
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
template <typename InputIterator>
void LoopbackConnection<UTMessageParameter>::add(
    InputIterator const& begin, InputIterator const& end)
{
	m_encoder(begin, end);
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
LoopbackConnection<UTMessageParameter>::receive_queue_type
LoopbackConnection<UTMessageParameter>::receive_all()
{
	receive_queue_type all;
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	std::swap(all, m_receive_queue);
	return all;
}

template <typename UTMessageParameter>
void LoopbackConnection<UTMessageParameter>::work_receive()
{
	while (m_run_receive) {
		std::lock_guard<std::mutex> lock(m_intermediate_queue_mutex);
		{
			std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
			m_decoder(m_intermediate_queue.begin(), m_intermediate_queue.end());
		}
		m_intermediate_queue.clear();
	}
}

template <typename UTMessageParameter>
bool LoopbackConnection<UTMessageParameter>::receive_empty() const
{
	std::unique_lock<std::mutex> lock(m_receive_queue_mutex);
	return m_receive_queue.empty();
}

} // namespace hxcomm
