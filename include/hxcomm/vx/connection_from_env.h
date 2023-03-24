#pragma once

#include "hate/visibility.h"
#include "hxcomm/vx/connection_variant.h"

#include <optional>
#include <vector>

namespace hxcomm::vx {

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is ZeroMockConnection > QuiggeldyConnection > Extoll-connected hardware >
 * HostARQ-connected hardware > CoSim right now if several are available.
 *
 * On the Python-side it is wrapped via ManagedConnection.
 *
 * @return An already allocated connection object.
 */
hxcomm::vx::ConnectionVariant get_connection_from_env() SYMBOL_VISIBLE;

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant list.
 *
 * Order of precedence is HardwareBackend > CoSim right now if both are available.
 *
 * @return A list of already allocated connection objects.
 */
std::vector<hxcomm::vx::ConnectionVariant> get_connection_list_from_env(
    std::optional<size_t> limit = std::nullopt) SYMBOL_VISIBLE;

/**
 * Get the connection from env and check if it supports the full stream interface.
 *
 * If so, return it wrapped in an optional, otherwise the optional is empty.
 *
 * @return Optional wrapping the connection object with support for the full
 * stream interface, empty otherwise.
 */
std::optional<hxcomm::vx::ConnectionFullStreamInterfaceVariant>
get_connection_full_stream_interface_from_env() SYMBOL_VISIBLE;

} // namespace hxcomm::vx
