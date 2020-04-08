
#include "hxcomm/common/connection_context.h"

#include "hxcomm/common/execute_messages.h"
#include "hxcomm/common/logger.h"

#include "hate/memory.h"

#include <tuple>

namespace hxcomm {

template <typename Connection>
template <typename... Args>
Proxy<Connection>::Proxy(Args&&... args) : m_connection(new Connection(std::forward<Args>(args)...))
{}

template <typename Connection>
Proxy<Connection>::~Proxy()
{
	this->clear();
}

#include <iostream>

template <typename Connection>
void Proxy<Connection>::clear()
{
    std::cerr << "Clearing connection!!!" << std::endl;
	m_connection = nullptr;
}

template <typename Connection>
std::optional<Connection*> Proxy<Connection>::get()
{
	if (m_connection) {
		return std::make_optional(m_connection.get());
	} else {
		return std::nullopt;
	}
}

template <typename Connection>
template <typename... Args>
Context<Connection>::Context(Args&&... args)
{
	if constexpr (sizeof...(Args) > 0) {
		m_init_params = std::make_tuple(std::forward<Args>(args)...);
	} else {
		m_init_params = std::nullopt;
	}
}

template <typename Connection>
Context<Connection>::~Context()
{
    if (m_proxy)
    {
        m_proxy->clear();
    }
}

template <typename Connection>
Context<Connection>::shared_proxy_type Context<Connection>::__enter__()
{
	if (m_proxy) {
		auto log = log4cxx::Logger::getLogger("hxcomm::Context");
		HXCOMM_LOG_WARN(log, "Context was already allocated! Clearing previous allocation..");
		m_proxy->clear();
	}
	if (m_init_params) {
		m_proxy = hate::memory::make_shared_from_tuple<proxy_type>(*m_init_params);
	} else {
		m_proxy = std::make_shared<proxy_type>();
	}

	return m_proxy;
}

template <typename Connection>
void Context<Connection>::__exit__()
{
    if (m_proxy)
    {
        m_proxy->clear(); // because a reference might still be held in python
    }
	m_proxy = nullptr;
}

namespace detail {

template <typename Connection, template <typename> class Sequence>
struct ExecutorMessages<std::shared_ptr<Proxy<Connection>>, Sequence>
{
	using connection_type = Connection;
	using proxy_type = Proxy<Connection>;
	using shared_proxy_type = std::shared_ptr<proxy_type>;
	using receive_message_type = typename connection_type::receive_message_type;
	using return_type = Sequence<receive_message_type>;
	using messages_type = Sequence<typename connection_type::send_message_type>;

	return_type operator()(shared_proxy_type& proxy, messages_type const& messages)
	{
		if (auto conn = proxy->get(); conn) {
			return execute_messages(**conn, messages);
		} else {
			throw std::runtime_error("Trying to execute messages on uninitialized connection.");
		}
	}
};

} // namespace detail
} // namespace hxcomm
