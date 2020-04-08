#pragma once

#include "hxcomm/common/execute_messages.h"

#include <memory>
#include <optional>

namespace hxcomm {

template <typename Connection>
class Context;

/**
 * Proxy object for connection emitted by connection context.
 *
 * Can be given to `execute_messages` in-place of the wrapped connection.
 */
template <typename Connection>
class Proxy
{
public:
	/**
	 *
	 * Construct the underyling connection with the given arguments.
	 *
	 * @tparam Args Arguments given to connection.
	 */
	template <typename... Args>
	Proxy(Args&&...);

	~Proxy();

    /**
     * Get a reference to the connection object if the proxy has stored one.
     *
     */
    std::optional<Connection*> get();

private:
	friend Context<Connection>;
	/**
	 * Clear connection early.
	 */
	void clear();

	std::unique_ptr<Connection> m_connection;
};

template <typename Connection>
class Context
{
public:
	using connection_type = Connection;
	using connection_init_parameters_type = typename connection_type::init_parameters_type;
	using proxy_type = Proxy<connection_type>;
	using shared_proxy_type = std::shared_ptr<proxy_type>;

	/**
	 * Create a context that will construct the underyling connection with given arguments upon
	 * entering.
	 *
	 * Upon exiting, the proxy object will be deleted.
	 *
	 * @tparam Args Arguments given to connection.
	 */
	template <typename... Args>
	Context(Args&&...);

    ~Context();

	/**
	 * Enter the context with allocated connection.
	 *
	 * @return Proxy-object handling the underlying connection.
	 */
	std::shared_ptr<proxy_type> __enter__();

	/**
	 * Destroys the connection in the current proxy object.
	 */
	void __exit__();

private:
	std::optional<connection_init_parameters_type> m_init_params;

	shared_proxy_type m_proxy;
};

} // namespace hxcomm

#include "hxcomm/common/connection_context.tcc"
