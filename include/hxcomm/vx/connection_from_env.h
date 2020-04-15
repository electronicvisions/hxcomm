#pragma once

#include "hxcomm/vx/connection_variant.h"

#include <optional>
#include <vector>

namespace hxcomm::vx {

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is QuiggeldyConnection > Real Hardware > CoSim right now if several are
 * available.
 *
 * On the Python-side it is wrapped via ManagedConnection.
 *
 * @return An already allocated connection object.
 */
inline hxcomm::vx::ConnectionVariant get_connection_from_env();

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant list.
 *
 * Order of precedence is HardwareBackend > CoSim right now if both are available.
 *
 * @return A list of already allocated connection objects.
 */
inline std::vector<hxcomm::vx::ConnectionVariant> get_connection_list_from_env(
    std::optional<size_t> limit = std::nullopt);

/**
 * Get the connection from env and check if it supports the full stream interface.
 *
 * If so, return it wrapped in an optional, otherwise the optional is empty.
 *
 * @return Optional wrapping the connection object with support for the full
 * stream interface, empty otherwise.
 */
inline std::optional<hxcomm::vx::ConnectionFullStreamInterfaceVariant>
get_connection_full_stream_interface_from_env();

} // namespace hxcomm::vx

#ifndef __GENPYBIND__
#include "connection_from_env.tcc"
#endif
