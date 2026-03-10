#include "hxcomm/common/multiconnection.h"

#include <future>

namespace hxcomm {

template <typename Connection>
MultiConnection<Connection>::MultiConnection()
{
	m_connections.emplace_back(Connection());
}


template <typename Connection>
MultiConnection<Connection>::MultiConnection(std::vector<Connection>&& connections) :
    m_connections(std::move(connections))
{
}


template <typename Connection>
MultiConnection<Connection>::MultiConnection(init_parameters_type const& connection_inits)
{
	for (auto const& connection_init : get<0>(connection_inits)) {
		m_connections.emplace_back(std::move(Connection(connection_init)));
	}
}


template <typename Connection>
MultiConnection<Connection>::MultiConnection(MultiConnection&& other) :
    m_connections(std::move(other.m_connections))
{
}


template <typename Connection>
MultiConnection<Connection>& MultiConnection<Connection>::operator=(MultiConnection&& other)
{
	if (this != &other) {
		m_connections = std::move(other.m_connections);
	}
	return *this;
}


template <typename Connection>
Connection& MultiConnection<Connection>::operator[](size_t index)
{
	return m_connections.at(index);
}

template <typename Connection>
size_t MultiConnection<Connection>::size() const
{
	return m_connections.size();
}


template <typename Connection>
std::vector<ConnectionTimeInfo> MultiConnection<Connection>::get_time_info() const
{
	std::vector<ConnectionTimeInfo> time_infos;
	for (auto const& connection : m_connections) {
		time_infos.push_back(connection.get_time_info());
	}

	return time_infos;
}


template <typename Connection>
std::vector<std::string> MultiConnection<Connection>::get_unique_identifier(
    std::optional<std::string> hwdb_path) const
{
	std::vector<std::string> unique_identifiers;

	for (auto const& connection : m_connections) {
		unique_identifiers.push_back(connection.get_unique_identifier(hwdb_path));
	}
	return unique_identifiers;
}


template <typename Connection>
std::vector<std::string> MultiConnection<Connection>::get_bitfile_info() const
{
	std::vector<std::string> bitfile_info;
	for (auto const& connection : m_connections) {
		bitfile_info.push_back(connection.get_bitfile_info());
	}

	return bitfile_info;
}


template <typename Connection>
std::vector<std::string> MultiConnection<Connection>::get_remote_repo_state() const
{
	std::vector<std::string> remote_repo_state;
	for (auto const& connection : m_connections) {
		remote_repo_state.push_back(connection.get_remote_repo_state());
	}

	return remote_repo_state;
}


template <typename Connection>
std::vector<HwdbEntry> MultiConnection<Connection>::get_hwdb_entry() const
{
	std::vector<HwdbEntry> entries;
	for (auto const& connection : m_connections) {
		entries.push_back(connection.get_hwdb_entry());
	}
	return entries;
}

} // namespace hxcomm