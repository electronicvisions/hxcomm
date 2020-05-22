#pragma once

#include <memory>
#include <utility>
#include <variant>

#include "hate/type_traits.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/visit_connection.h"

namespace hxcomm {

namespace detail {

/**
 * Helper for partial specialization over templated connection-types.
 *
 * operator()-contains the implementation of execute_messages defined below.
 *
 * TODO: Validate interface via concepts once gcc 10 is ready.
 */
template <typename Connection, template <typename> class Sequence>
struct ExecutorMessages
{
	using connection_type = Connection;
	using receive_message_type = typename connection_type::receive_message_type;
	using send_message_type = typename connection_type::send_message_type;
	using response_type = Sequence<receive_message_type>;
	using return_type = std::pair<response_type, ConnectionTimeInfo>;
	using messages_type = Sequence<send_message_type>;
	using send_halt_message_type = typename connection_type::send_halt_message_type;

	static_assert(
	    hate::is_detected_v<ConnectionConcept, connection_type>,
	    "Connection does not adhere to ConnectionConcept.");

	return_type operator()(connection_type& conn, messages_type const& messages)
	{
		Stream<connection_type> stream(conn);
		auto const time_begin = conn.get_time_info();

		stream.add(messages);
		stream.add(send_halt_message_type());
		stream.commit();

		stream.run_until_halt();

		response_type responses;
		receive_message_type response;
		while (stream.try_receive(response)) {
			responses.push_back(response);
		}

		auto const time_difference = conn.get_time_info() - time_begin;
		return {responses, time_difference};
	}
};

} // namespace detail

/**
 * Execute the given messages on the given connection.
 *
 * This function is specialized for each architecture in the corresponding
 * `hxcomm/<architecture>/execute_messages.h` header or - if needed - in the
 * connection-header itself.
 * This function makes use of Stream, which locks the connection by acquiring the mutex via
 * connection.get_mutex() to ensure mutual exclusion of access.
 *
 * @tparam Connection The connection on which the messages are executed.
 * @tparam Sequence In which sequential container should the messages be stored.
 */
template <
    typename Connection,
    template <typename>
    class Sequence,
    ConnectionIsPlainGuard<Connection> = 0>
std::pair<Sequence<typename GetMessageTypes<Connection>::type::receive_type>, ConnectionTimeInfo>
execute_messages(
    Connection& connection,
    Sequence<typename GetMessageTypes<Connection>::type::send_type> const& messages)
{
	auto const [res, time] = detail::ExecutorMessages<Connection, Sequence>()(connection, messages);
	[[maybe_unused]] log4cxx::Logger* log = log4cxx::Logger::getLogger("hxcomm.execute_messages");
	HXCOMM_LOG_INFO(
	    log, "Executed messages(" << messages.size() << ") and got responses(" << res.size()
	                              << ") with time expenditure: " << std::endl
	                              << time << ".");
	return {res, time};
}

template <
    typename Connection,
    template <typename>
    class Sequence,
    ConnectionIsWrappedGuard<Connection> = 0>
std::pair<
    Sequence<typename GetMessageTypes<std::remove_cvref_t<Connection>>::type::receive_type>,
    ConnectionTimeInfo>
execute_messages(
    Connection&& connection,
    Sequence<typename GetMessageTypes<std::remove_cvref_t<Connection>>::type::send_type> const&
        messages)
{
	return hxcomm::visit_connection(
	    [&messages](auto& conn) -> decltype(auto) { return execute_messages(conn, messages); },
	    std::forward<Connection>(connection));
}

} // namespace hxcomm
