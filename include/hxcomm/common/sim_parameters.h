#pragma once
#include "hate/visibility.h"
#include <cstdint>
#include <string>
#include <tuple>

namespace hxcomm {

/**
 * Get simulator IP and port via the environment.
 * @return Parameters
 */
std::tuple<std::string, uint16_t> get_sim_parameters() SYMBOL_VISIBLE;

} // namespace hxcomm
