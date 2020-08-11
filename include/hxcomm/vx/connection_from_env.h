#pragma once

#include "hxcomm/vx/connection_variant.h"

#include <vector>

namespace hxcomm::vx {

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is HardwareBackend > CoSim right now if both are available.
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

} // namespace hxcomm::vx

#ifndef __GENPYBIND__
#include "connection_from_env.tcc"
#endif
