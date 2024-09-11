#pragma once
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/execute_messages_types.h"
#include "hxcomm/common/logger.h"
#include "hxcomm/common/stream.h"
#include "hxcomm/common/visit_connection.h"
#include <memory>
#include <utility>
#include <variant>

#include "hate/type_traits.h"

#include <memory>
#include <utility>
#include <variant>

namespace hxcomm {

namespace detail {

/**
 * Helper for partial specialization over templated connection-types.
 *
 * operator()-contains the implementation of execute_messages defined below.
 *
 * TODO: Validate interface via concepts once gcc 10 is ready.
 */
template <typename Connection>
struct ExecutorMessages
{
	using connection_type = Connection;
	using receive_message_type = typename connection_type::receive_message_type;
	using send_message_type = typename connection_type::send_message_type;
	using return_type = execute_messages_return_t<Connection>;
	using response_type = typename return_type::first_type;
	using messages_type = execute_messages_argument_t<Connection>;
	using send_halt_message_type = typename connection_type::send_halt_message_type;

	static_assert(
	    hate::is_detected_v<ConnectionConcept, connection_type>,
	    "Connection does not adhere to ConnectionConcept.");

	return_type operator()(
	    connection_type& conn, messages_type const& messages, bool const keep_responses)
	{
		Stream<connection_type> stream(conn);
		auto const time_begin = conn.get_time_info();

		stream.add(messages.begin(), messages.end());
		stream.add(send_halt_message_type());
		stream.commit();

		stream.run_until_halt();

		if (keep_responses) {
			auto const time_difference = conn.get_time_info() - time_begin;
			return {{}, time_difference};
		} else {
			auto const responses = stream.receive_all();
			auto const time_difference = conn.get_time_info() - time_begin;
			return {responses, time_difference};
		}
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
template <typename Connection, ConnectionIsPlainGuard<Connection> = 0>
detail::execute_messages_return_t<Connection> execute_messages(
    Connection& connection,
    detail::execute_messages_argument_t<Connection> const& messages,
    bool const keep_responses = false)
{
	auto const [res, time] =
	    detail::ExecutorMessages<Connection>()(connection, messages, keep_responses);
	[[maybe_unused]] log4cxx::LoggerPtr log = log4cxx::Logger::getLogger("hxcomm.execute_messages");
	HXCOMM_LOG_INFO(
	    log, "Executed messages(" << messages.size() << ") and got responses(" << res.size()
	                              << ") with time expenditure: " << std::endl
	                              << time << ".");
	return std::pair{res, time};
}

template <typename Connection, ConnectionIsWrappedGuard<Connection> = 0>
detail::execute_messages_return_t<Connection> execute_messages(
    Connection&& connection,
    detail::execute_messages_argument_t<Connection> const& messages,
    bool const keep_responses = false)
{
	return hxcomm::visit_connection(
	    [&messages, keep_responses](auto& conn) -> decltype(auto) {
		    return execute_messages(conn, messages, keep_responses);
	    },
	    std::forward<Connection>(connection));
}

} // namespace hxcomm
