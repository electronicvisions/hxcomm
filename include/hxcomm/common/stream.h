#pragma once

#include <boost/variant.hpp>
#include "hate/type_list.h"

#include "hxcomm/common/connection.h"

#include <mutex>
#include <type_traits>
#include <vector>

namespace hxcomm {

/**
 * Interface definition as to how each connection can be interacted with.
 *
 * The user is supposed to only be able to construct and destroy `Connection`
 * handles on its own. Interaction is mitigated by the corresponding
 * `Stream<Connection>`-object which is constructed by `execute_messages`
 *
 * Note: The Stream<Connection>-interface is intended for hxcomm and its
 * internal testing routines and not meant to be relied upon in upper layers!
 *
 * Instead, interaction with upper layers happens via
 * `execute_messages(connection, messages)`. Here the specialized versions of
 * `Stream<Connection>` that have to be derived from this class are
 * instantiated.
 *
 * By convention, `Stream<Connection>` is befriended by `Connection` and calls
 * private functions of the same signature as its interface.
 *
 * @tparam Connection Which connection object to use for operation.
 */
template <typename Connection>
class Stream
{
public:
	using connection_type = Connection;

	using message_types = typename GetMessageTypes<Connection>::type;

	using receive_message_type = typename message_types::receive_type;
	using send_message_type = typename message_types::send_type;

	template <typename ConnType>
	struct Streamable
	{
		template <typename MessageT>
		using has_add_method = typename std::is_same<
		    decltype(std::declval<ConnType>().add(std::declval<MessageT const&>())),
		    void>::type;

		template <typename>
		struct has_templated_add_method : std::false_type
		{};

		template <typename... MessageTs>
		struct has_templated_add_method<boost::variant<MessageTs...>>
		    : std::conjunction<has_add_method<MessageTs>...>
		{};

		static_assert(
		    has_templated_add_method<send_message_type>::value,
		    "Connection add-method not defined for all messages.");

		static_assert(
		    has_add_method<typename std::vector<send_message_type>>::value,
		    "Connection missing add-method for vectors.");

		static_assert(
		    std::is_same_v<decltype(&ConnType::commit), void (ConnType::*)()>,
		    "Connection missing commit-method.");

		static_assert(
		    std::is_same_v<decltype(&ConnType::receive), receive_message_type (ConnType::*)()>,
		    "Connection missing receive-method.");

		static_assert(
		    std::is_same_v<
		        decltype(&ConnType::try_receive),
		        bool (ConnType::*)(receive_message_type&)>,
		    "Connection missing try_receive-method.");

		static_assert(
		    std::is_same_v<decltype(&ConnType::run_until_halt), void (ConnType::*)()>,
		    "Connection missing run_until_halt-method.");

		static_assert(
		    std::is_same_v<decltype(&ConnType::get_mutex), std::mutex& (ConnType::*) ()>,
		    "Connection does not have a get_mutex-method.");
	};

	static_assert(sizeof(Streamable<Connection>) > 0, "Connection adhere to Stream-Interface.");

	/**
	 * Construct a Stream for the given connection handle which it manages.
	 * Lock connection-internal mutex to ensure mutual exclusion.
	 */
	Stream(connection_type& conn) : m_connection(conn), m_connection_lock(conn.get_mutex()){};

	/**
	 * There can only be one Stream per connection.
	 */
	Stream(Stream const&) = delete;

	/**
	 * There can only be one Stream per connection.
	 */
	Stream& operator=(Stream const&) = delete;

	/**
	 * Add a single UT message to the send queue.
	 * @param message Message to add
	 */
	template <typename MessageType>
	void add(MessageType const& message)
	{
		m_connection.add(message);
	}

	/**
	 * Add multiple UT messages to the send queue.
	 * @param messages Messages to add
	 */
	void add(std::vector<send_message_type> const& messages)
	{
		m_connection.add(messages);
	}

	/**
	 * Send messages in send queue.
	 */
	void commit()
	{
		m_connection.commit();
	}

	/**
	 * Receive a single UT message.
	 * @throws std::runtime_error On empty message queue
	 * @return Received message
	 */
	receive_message_type receive()
	{
		return m_connection.receive();
	}

	/**
	 * Try to receive a single UT message.
	 * @param message Message to receive to
	 * @return Boolean value whether receive was successful
	 */
	bool try_receive(receive_message_type& message)
	{
		return m_connection.try_receive(message);
	}

	/**
	 * Get whether the connection has no UT messages available to receive.
	 * @return Boolean value
	 */
	bool receive_empty() const
	{
		return m_connection.receive_empty();
	}

	/**
	 * Start emulation and wait until halt instruction is received from simulation.
	 */
	void run_until_halt()
	{
		return m_connection.run_until_halt();
	};

protected:
	connection_type& m_connection;
	std::unique_lock<std::mutex> m_connection_lock;
};

} // namespace hxcomm