#pragma once

#include "hxcomm/common/listener_seen.h"
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace hxcomm {

/**
 * Wrapper around ListenerSeen to offload check in a separate thread at the
 * cost of working on copies.
 * Filtering is done at compile-time for the templated message type.
 * Payload is not looked at.
 * @see ListenerSeen
 * @tparam ContainerType Container to be checked. Typically vector of message
 *                       variants. But any iterable container will suffice.
 * @tparam SeenMessageType Message type that should be checked for.
 */
template <typename SeenMessageType, typename ContainerType>
class ListenerSeenThread
{
public:
	ListenerSeenThread();
	~ListenerSeenThread();
	ListenerSeenThread(const ListenerSeenThread&) = delete;
	ListenerSeenThread(ListenerSeenThread&&) = delete;

	/**
	 * Submit a given container (typically a vector of UTMessages) to be
	 * checked.
	 *
	 * @param Container to be checked.
	 */
	void submit_check(ContainerType);

	/**
	 * Block until all checks submitted so far are performed.
	 *
	 * @return Whether the given message type was detected.
	 */
	bool get_check_result();

	/**
	 * Reset listener state to not having registered a templated message type.
	 */
	void reset();

protected:
	std::unique_lock<std::mutex> lock() const;
	std::lock_guard<std::mutex> lock_guard() const;

	void thread_loop(std::stop_token);

	using listener_timeout_type = ListenerSeen<SeenMessageType>;
	listener_timeout_type m_listener_timeout;

	log4cxx::Logger* m_logger;

	std::deque<ContainerType> m_deque;
	mutable std::mutex m_mutex;
	std::condition_variable m_cv;

	std::jthread m_thread;
};

} // namespace hxcomm

#include "hxcomm/common/listener_seen_thread.tcc"
