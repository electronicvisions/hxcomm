#include "hxcomm/common/connection.h"
#include "hxcomm/common/logger.h"
#include "pyhxcomm/common/getname.h"
#include "pyhxcomm/common/handle_connection.h"
#include <boost/hana/string.hpp>

namespace hxcomm {

template <typename C>
struct GetMessageTypes<pyhxcomm::Handle<C>>
{
	using type = typename GetMessageTypes<C>::type;
};

} // namespace hxcomm


namespace pyhxcomm {

using namespace boost::hana::literals;

template <typename Connection>
template <typename... Args>
Handle<Connection>::Handle(Args&&... args) :
    m_connection(new Connection(std::forward<Args>(args)...))
{
	setup_logger();
}

template <typename Connection>
Handle<Connection>::Handle(Handle&& other) : m_connection(other.m_connection.release())
{
	setup_logger();
}

template <typename Connection>
Handle<Connection>::Handle(Connection&& connection) :
    m_connection(new Connection(std::move(connection)))
{
	setup_logger();
}

template <typename Connection>
Handle<Connection>::~Handle()
{
	disconnect();
}

template <typename Connection>
void Handle<Connection>::setup_logger()
{
	m_logger = log4cxx::Logger::getLogger(
	    ("pyhxcomm."_s + GetName<Connection>::name() + "Handle"_s).c_str());
}

template <typename Connection>
void Handle<Connection>::disconnect()
{
	if (m_connection) {
		HXCOMM_LOG_TRACE(m_logger, "Disconnecting..");
		m_connection.reset();
	}
}

template <typename Connection>
Connection& Handle<Connection>::get()
{
	if (m_connection) {
		return *m_connection.get();
	} else {
		HXCOMM_LOG_ERROR(m_logger, "Connection is not allocated!");
		throw std::runtime_error("Connection is not allocated!");
	}
}

template <typename Connection>
Connection const& Handle<Connection>::get() const
{
	if (m_connection) {
		return *m_connection.get();
	} else {
		HXCOMM_LOG_ERROR(m_logger, "Connection is not allocated!");
		throw std::runtime_error("Connection is not allocated!");
	}
}

template <typename Connection>
std::unique_ptr<Connection> Handle<Connection>::release()
{
	if (m_connection) {
		return std::move(m_connection);
	} else {
		HXCOMM_LOG_ERROR(m_logger, "Connection is not allocated!");
		throw std::runtime_error("Connection is not allocated!");
	}
}

} // namespace pyhxcomm
