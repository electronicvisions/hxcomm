#pragma once

#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/genpybind.h"

namespace hxcomm::vx GENPYBIND_TAG_HXCOMM_VX {

/**
 * Automatically determine from environment what connection type to use and
 * return the corresponding variant.
 *
 * Order of precedence is HardwareBackend > CoSim right now if both are available.
 *
 * @return An already allocated connection object.
 */
inline hxcomm::vx::ConnectionVariantType get_connection_from_env() GENPYBIND(visible);

} // namespace hxcomm::vx


#ifndef __GENPYBIND__
#include "connection_from_env.tcc"
#endif
