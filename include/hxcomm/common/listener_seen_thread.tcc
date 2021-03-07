
#include <chrono>

namespace hxcomm {

template <typename SeenMessageType, typename ContainerType>
ListenerSeenThread<SeenMessageType, ContainerType>::ListenerSeenThread() :
    m_logger(log4cxx::Logger::getLogger("hxcomm.ListenerSeenThread")),
    m_thread{[this](std::stop_token st) { thread_loop(std::move(st)); }}
{}

template <typename SeenMessageType, typename ContainerType>
ListenerSeenThread<SeenMessageType, ContainerType>::~ListenerSeenThread()
{
	HXCOMM_LOG_TRACE(m_logger, "~ListenerSeenThread");
	using namespace std::chrono_literals;
	m_thread.request_stop();
	auto lk = lock();
	m_cv.notify_all();
	lk.unlock();
	m_thread.join();
}

template <typename SeenMessageType, typename ContainerType>
void ListenerSeenThread<SeenMessageType, ContainerType>::thread_loop(std::stop_token st)
{
	HXCOMM_LOG_TRACE(m_logger, "Starting up.");
	auto lk = lock();
	while (!st.stop_requested()) {
		try {
			HXCOMM_LOG_TRACE(m_logger, "Waiting for new checks.");
			m_cv.wait(lk, [&] { return st.stop_requested() || !m_deque.empty(); });
			HXCOMM_LOG_TRACE(m_logger, "New checks received.");
			if (st.stop_requested()) {
				break;
			}
			while (!m_deque.empty()) {
				auto const& current = m_deque.front();
				std::for_each(current.cbegin(), current.cend(), [this](auto const& msg) {
					std::visit([this](auto const& m) { m_listener_timeout(m); }, msg);
				});
				m_deque.pop_front();
			}
		} catch (std::exception const& e) {
			HXCOMM_LOG_ERROR(m_logger, "Exception: " << e.what());
		}
	}
	HXCOMM_LOG_TRACE(m_logger, "Shutting down.");
}

template <typename SeenMessageType, typename ContainerType>
void ListenerSeenThread<SeenMessageType, ContainerType>::submit_check(ContainerType to_check)
{
	HXCOMM_LOG_TRACE(m_logger, "Submitting check.");
	{
		auto lk = lock_guard();
		m_deque.push_back(std::move(to_check));
	}
	m_cv.notify_all();
	HXCOMM_LOG_TRACE(m_logger, "Submitted check.");
}

template <typename SeenMessageType, typename ContainerType>
bool ListenerSeenThread<SeenMessageType, ContainerType>::get_check_result()
{
	HXCOMM_LOG_TRACE(m_logger, "Getting check results..");
	auto lk = lock();
	while (!m_deque.empty()) {
		lk.unlock();
		HXCOMM_LOG_TRACE(m_logger, "Still checks to be performed.");
		m_cv.notify_one();
		lk.lock();
	}
	HXCOMM_LOG_TRACE(
	    m_logger, "Message type " << (m_listener_timeout.get() ? "" : "not ") << "seen.");
	return m_listener_timeout.get();
}

template <typename SeenMessageType, typename ContainerType>
void ListenerSeenThread<SeenMessageType, ContainerType>::reset()
{
	auto const lock_guard();
	m_listener_timeout.reset();
}

template <typename SeenMessageType, typename ContainerType>
std::unique_lock<std::mutex> ListenerSeenThread<SeenMessageType, ContainerType>::lock() const
{
	return std::unique_lock{m_mutex};
}

template <typename SeenMessageType, typename ContainerType>
std::lock_guard<std::mutex> ListenerSeenThread<SeenMessageType, ContainerType>::lock_guard() const
{
	return std::lock_guard{m_mutex};
}

} // namespace hxcomm
