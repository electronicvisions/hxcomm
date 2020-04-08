#pragma once

#include "hxcomm/common/logger.h"

#include <memory>

namespace log4cxx {

class Logger;

} // namespace log4cxx

namespace pyhxcomm {

template <typename Connection>
class Managed;

/**
 * Handle object for connection emitted by connection managed.
 *
 * Can be given to `execute_messages` in-place of the wrapped connection.
 */
template <typename Connection>
class Handle
{
public:
	using connection_type = Connection;

	/**
	 * Construct the underlying connection with the given arguments.
	 *
	 * @tparam Args Arguments given to connection.
	 */
	template <typename... Args>
	Handle(Args&&...);

	/**
	 * Move constructor.
	 */
	Handle(Handle&&);

	/**
	 * Create Handle from existing Connection rvalue.
	 */
	Handle(connection_type&&);

	/**
	 * No copy as one handle object holds one connection.
	 */
	Handle(Handle const&) = delete;

	/**
	 * No copy as one handle object holds one connection.
	 */
	Handle& operator=(Handle const&) = delete;

	/**
	 * Destructor asserts that the connection is cleared.
	 */
	~Handle();

	/**
	 * Get a reference to the connection object if the handle has stored one,
	 * otherwise a runtime error will be thrown.
	 */
	connection_type& get();

private:
	// needed for disconnect()-access
	template <typename>
	friend class Managed;

	void setup_logger();

	log4cxx::Logger* m_logger;

	/**
	 * Disconnect (i.e. destruct) connection early.
	 */
	void disconnect();

	std::unique_ptr<connection_type> m_connection;
};

} // namespace pyhxcomm

#include "pyhxcomm/common/handle_connection.tcc"
