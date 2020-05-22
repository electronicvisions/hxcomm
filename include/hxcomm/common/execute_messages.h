#pragma once

#include <memory>
#include <variant>

#include "hate/type_traits.h"
#include "hxcomm/common/connection.h"
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
	using return_type = Sequence<receive_message_type>;
	using messages_type = Sequence<send_message_type>;
	using send_halt_message_type = typename connection_type::send_halt_message_type;

	static_assert(
	    hate::is_detected_v<ConnectionConcept, connection_type>,
	    "Connection does not adhere to ConnectionConcept.");

	return_type operator()(connection_type& conn, messages_type const& messages)
	{
		Stream<connection_type> stream(conn);

		stream.add(messages);
		stream.add(send_halt_message_type());
		stream.commit();

		stream.run_until_halt();

		return_type responses;
		receive_message_type response;
		while (stream.try_receive(response)) {
			responses.push_back(response);
		}
		return responses;
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
Sequence<typename GetMessageTypes<Connection>::type::receive_type> execute_messages(
    Connection& connection,
    Sequence<typename GetMessageTypes<Connection>::type::send_type> const& messages)
{
	return detail::ExecutorMessages<Connection, Sequence>()(connection, messages);
}

template <
    typename Connection,
    template <typename>
    class Sequence,
    ConnectionIsWrappedGuard<Connection> = 0>
Sequence<typename GetMessageTypes<std::remove_cvref_t<Connection>>::type::receive_type>
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
