namespace hxcomm {

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::SimConnection(ip_t ip, port_t port) :
    m_sim(ip, port),
    m_send_queue(),
    m_encoder(m_send_queue),
    m_receive_queue(),
    m_listener_halt(),
    m_decoder(m_receive_queue, m_listener_halt),
    m_run_receive(true),
    m_receive_buffer(m_run_receive),
    m_worker_fill_receive_buffer(
        &SimConnection<ConnectionParameter>::work_fill_receive_buffer, this, ip, port),
    m_worker_decode_messages(&SimConnection<ConnectionParameter>::work_decode_messages, this)
{}

template <typename ConnectionParameter>
SimConnection<ConnectionParameter>::~SimConnection()
{
	m_run_receive = false;
	m_receive_buffer.notify();
	m_worker_fill_receive_buffer.join();
	m_worker_decode_messages.join();
}

template <typename ConnectionParameter>
template <class MessageType>
void SimConnection<ConnectionParameter>::add(MessageType const& message)
{
	m_encoder(message);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::add(std::vector<send_message_type> const& messages)
{
	m_encoder(messages);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::commit()
{
	m_encoder.flush();
	while (!m_send_queue.empty()) {
		m_sim.send(m_send_queue.front());
		m_send_queue.pop();
	}
}

template <typename ConnectionParameter>
typename SimConnection<ConnectionParameter>::receive_message_type
SimConnection<ConnectionParameter>::receive()
{
	receive_message_type message;
	if (__builtin_expect(!m_receive_queue.try_pop(message), false)) {
		throw std::runtime_error("No message available to receive.");
	}
	return message;
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::try_receive(receive_message_type& message)
{
	return m_receive_queue.try_pop(message);
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::work_fill_receive_buffer(ip_t ip, port_t port)
{
	thread_local decltype(m_sim) local_sim(ip, port);

	while (true) {
		auto const write_pointer = m_receive_buffer.start_write();
		if (!write_pointer)
			return;
		{
			size_t words_written = 0;
			while (local_sim.receive_data_available() && (words_written < receive_buffer_size)) {
				write_pointer->data[words_written] = local_sim.receive();
				words_written++;
			}
			write_pointer->set_size(words_written);
		}
		m_receive_buffer.stop_write();
		if (!m_run_receive.load(std::memory_order_acquire))
			return;
	}
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::work_decode_messages()
{
	while (true) {
		auto const read_pointer = m_receive_buffer.start_read();
		if (!read_pointer)
			return;
		m_decoder(*read_pointer);
		m_receive_buffer.stop_read();
	}
}

template <typename ConnectionParameter>
bool SimConnection<ConnectionParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::run_for(flange::SimulatorEvent::clk_t clock)
{
	std::unique_lock<std::mutex> lock(m_runnable_mutex);
	lock.lock();
	if (m_sim.get_runnable()) {
		throw std::runtime_error("Trying to start already running simulation.");
	}

	flange::SimulatorEvent::clk_t current_time = m_sim.get_current_time();

	// start simulation
	m_sim.set_runnable(true);
	lock.unlock();

	constexpr size_t wait_period = 10000;
	constexpr size_t timeout = 60 * 1000 * 1000 / wait_period;
	size_t counter = 0;
	while (m_sim.get_runnable()) {
		usleep(wait_period);
		if (counter++ > timeout) {
			throw std::runtime_error(
			    "Timeout while waiting for simulation to progress to clock cycle.");
		}
		if (m_sim.get_current_time() >= current_time + clock) {
			break;
		}
	}

	lock.lock();
	m_sim.set_runnable(false);
	lock.unlock();
}

template <typename ConnectionParameter>
void SimConnection<ConnectionParameter>::run_until_halt()
{
	std::unique_lock<std::mutex> lock(m_runnable_mutex);
	lock.lock();
	if (m_sim.get_runnable()) {
		throw std::runtime_error("Trying to start already running simulation.");
	}

	m_listener_halt.reset();

	// start simulation
	m_sim.set_runnable(true);
	lock.unlock();

	constexpr size_t wait_period = 10000;
	constexpr size_t timeout = 60 * 1000 * 1000 / wait_period;
	size_t counter = 0;
	while (!m_listener_halt.get()) {
		usleep(wait_period);
		if (counter++ > timeout) {
			throw std::runtime_error("Timeout while waiting for simulation to halt.");
		}
	}

	lock.lock();
	m_sim.set_runnable(false);
	lock.unlock();
	m_listener_halt.reset();
}

} // namespace hxcomm
