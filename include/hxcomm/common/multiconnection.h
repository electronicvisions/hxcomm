#pragma once

#include "hate/visibility.h"
#include "hxcomm/common/connection.h"
#include "hxcomm/common/connection_time_info.h"
#include "hxcomm/common/hwdb_entry.h"
#include <array>
#include <optional>
#include <string>
#include <vector>


namespace hxcomm {

/**
 * Vectoriced connection.
 * Holds multiple connections of a single type.
 */
template <typename Connection>
struct SYMBOL_VISIBLE MultiConnection
{
	// Concatenate name of MultiConnection and contained connection type.
	static const std::string name;

	using sub_connection_type = Connection;

	// tuple required for forwarding parameters in pywrapping.
	using init_parameters_type = std::tuple<std::vector<typename Connection::init_parameters_type>>;

	using message_types = typename Connection::message_types;

	using receive_message_type = std::vector<typename message_types::receive_type>;
	using send_message_type = std::vector<typename message_types::send_type>;
	using send_halt_message_type = std::vector<typename message_types::send_halt_type>;
	using receive_queue_type = std::vector<receive_message_type>;

	static constexpr auto supported_targets = Connection::supported_targets;

	/**
	 * Default constructor creates a single connection of its type.
	 */
	MultiConnection();

	/**
	 * Create MultiConnection (to FPGAs or Simulation) with address found in environment.
	 * Simple vectorization of connections.
	 * @param connections Connections which should be part of MultiConnection.
	 */
	MultiConnection(std::vector<Connection>&& connections);

	/**
	 * Create Multiconnection from given vector of tuples.
	 * @param connection_inits List of connection inits for the contained SingleConnections.
	 */
	MultiConnection(init_parameters_type const& connection_inits);

	/**
	 * Copy constructor (Deleted because no two instances with the same allocation can coexist).
	 */
	MultiConnection(MultiConnection const&) = delete;

	/**
	 *  Copy assignemnt operator (Deleted because no two instances witht the same allocation can
	 * coexist).
	 */
	MultiConnection& operator=(MultiConnection const&) = delete;

	/**
	 * Move constructor.
	 */
	MultiConnection(MultiConnection&& other);

	/**
	 * Move assignment operator.
	 */
	MultiConnection& operator=(MultiConnection&& other);

	/**
	 * Destructor.
	 */
	~MultiConnection() = default;

	/**
	 * Comparison Operator.
	 */
	// auto operator<=>(MultiConnection<Connection> const& other) const = default; TO-DO

	/**
	 * Return reference to connection.
	 */
	Connection& operator[](size_t index);

	/**
	 * Return number of contained connections.
	 * @return Number of contained connections.
	 */
	size_t size() const;

	/**
	 * Get time information.
	 * @return Time information of all connections.
	 */
	std::vector<ConnectionTimeInfo> get_time_info() const;

	/**
	 * Get unique identifiers from hwdb.
	 * @param hwdb_path Path to hwdb.
	 * @return Unique identifier.
	 */
	std::vector<std::string> get_unique_identifier(
	    std::optional<std::string> hwdb_path = std::nullopt) const;

	/**
	 * Get bitfile information.
	 * @return Bitfile info.
	 */
	std::vector<std::string> get_bitfile_info() const;

	/**
	 * Get server-side remote repository state information.
	 * Only non empty elements for QuiggeldyConnection.
	 * @return Repositroy State.
	 */
	std::vector<std::string> get_remote_repo_state() const;


	/**
	 * Get hwdb-entry of all connections.
	 * @return List of hwdb-entry for all connections.
	 */
	std::vector<HwdbEntry> get_hwdb_entry() const;

private:
	std::vector<Connection> m_connections;
};

template <typename Connection>
const std::string MultiConnection<Connection>::name = std::string("Multi") + Connection::name;

namespace detail {

template <typename Connection>
struct ExecutorMessages;

template <typename Connection>
struct ExecutorMessages<MultiConnection<Connection>>;

} // namespace detail

} // namespace hxcomm

#include "hxcomm/common/multiconnection.tcc"