#include "hate/timer.h"
#include "hate/variant.h"

namespace hxcomm {

template <typename ConnectionParameter>
ZeroMockConnection<ConnectionParameter>::ZeroMockConnection(long const ns_per_message) :
    m_send_queue(),
    m_receive_queue(),
    m_halt(false),
    m_ns_per_message(ns_per_message),
    m_process_message(m_receive_queue, m_halt),
    m_time_info(),
    m_last_time_info(),
    m_last_message_count(0)
{}


template <typename ConnectionParameter>
ZeroMockConnection<ConnectionParameter>::ZeroMockConnection(ZeroMockConnection&& other) :
    m_send_queue(std::move(other.m_send_queue)),
    m_receive_queue(std::move(other.m_receive_queue)),
    m_halt(other.m_halt),
    m_ns_per_message(other.m_ns_per_message),
    m_process_message(m_receive_queue, m_halt),
    m_time_info(other.m_time_info),
    m_last_time_info(other.m_last_time_info),
    m_last_message_count(other.m_last_message_count)
{}

template <typename ConnectionParameter>
ZeroMockConnection<ConnectionParameter>& ZeroMockConnection<ConnectionParameter>::operator=(
    ZeroMockConnection&& other)
{
	m_send_queue = std::move(other.m_send_queue);
	m_receive_queue = std::move(other.m_receive_queue);
	m_halt = other.m_halt;
	m_ns_per_message = other.m_ns_per_message;
	m_time_info = other.m_time_info;
	m_last_time_info = other.m_last_time_info;
	m_last_message_count = other.m_last_message_count;
	return *this;
}

template <typename ConnectionParameter>
std::mutex& ZeroMockConnection<ConnectionParameter>::get_mutex()
{
	return m_mutex;
}

template <typename ConnectionParameter>
ConnectionTimeInfo ZeroMockConnection<ConnectionParameter>::get_time_info() const
{
	return m_time_info;
}

template <typename ConnectionParameter>
std::string ZeroMockConnection<ConnectionParameter>::get_unique_identifier(
    std::optional<std::string> /* hwdb_path */) const
{
	// TODO: make unique
	return "zeromock";
}

template <typename ConnectionParameter>
std::string ZeroMockConnection<ConnectionParameter>::get_bitfile_info() const
{
	return "zeromock";
}

template <typename ConnectionParameter>
std::string ZeroMockConnection<ConnectionParameter>::get_remote_repo_state() const
{
	return "";
}

template <typename ConnectionParameter>
void ZeroMockConnection<ConnectionParameter>::add(send_message_type const& message)
{
	hate::Timer timer;
	m_process_message(message);
	m_last_message_count++;
	std::chrono::nanoseconds duration(timer.get_ns());
	m_time_info.execution_duration += duration;
}

template <typename ConnectionParameter>
void ZeroMockConnection<ConnectionParameter>::commit()
{
	// nothing to do here
}

template <typename ConnectionParameter>
typename ZeroMockConnection<ConnectionParameter>::receive_queue_type
ZeroMockConnection<ConnectionParameter>::receive_all()
{
	receive_queue_type all;
	std::swap(all, m_receive_queue);
	return all;
}

template <typename ConnectionParameter>
bool ZeroMockConnection<ConnectionParameter>::receive_empty() const
{
	return m_receive_queue.empty();
}


template <typename ConnectionParameter>
void ZeroMockConnection<ConnectionParameter>::run_until_halt()
{
	hate::Timer timer;

	if (!m_halt) {
		throw std::runtime_error("Reached end of program without halt instruction!");
	}

	m_halt = false;

	long const expected = m_last_message_count * m_ns_per_message;
	m_last_message_count = 0;

	long const until_now =
	    (m_time_info.execution_duration - m_last_time_info.execution_duration).count();
	long const left_ns = expected - until_now;

	// wait until time spent matches Messages rate
	while (timer.get_ns() < left_ns) {
	}
	m_time_info.execution_duration += std::chrono::nanoseconds(timer.get_ns());
	m_last_time_info = m_time_info;
}

} // namespace hxcomm
