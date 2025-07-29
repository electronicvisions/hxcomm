#pragma once

#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/single_connection_variant.h"

#include <optional>
#include <vector>

namespace hxcomm::vx {

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is ZeroMockConnection > QuiggeldyConnection >
 * HostARQ-connected hardware > CoSim right now if several are available.
 *
 * @param number_connections Number of connections of same type that are selected.
 *
 * @return An already allocated MultiConnection object.
 */
hxcomm::vx::ConnectionVariant get_connection_from_env(size_t number_connections = 1) SYMBOL_VISIBLE;

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is ZeroMockConnection > QuiggeldyConnection >
 * HostARQ-connected hardware > CoSim right now if several are available.
 *
 * @param number_connections_per_multi How many single-connections are merged into one
multi-connection. The number of resulting multi-connections is rounded off.
 *
 * @return An already allocated MultiConnection object.
 */
std::vector<hxcomm::vx::ConnectionVariant> get_connection_list_from_env(
    size_t number_connections_per_multi = 1) SYMBOL_VISIBLE;

/**
 * Get the connection from env and check if it supports the full stream interface.
 *
 * If so, return it wrapped in an optional, otherwise the optional is empty.
 *
 * @return Optional wrapping the connection object with support for the full
 * stream interface, empty otherwise.
 */
std::optional<hxcomm::vx::SingleConnectionFullStreamInterfaceVariant>
get_connection_full_stream_interface_from_env() SYMBOL_VISIBLE;

} // namespace hxcomm::vx
