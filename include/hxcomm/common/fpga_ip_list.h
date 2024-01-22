#pragma once
#include "hate/visibility.h"
#include <optional>
#include <string>
#include <vector>

namespace hxcomm {

/**
 * Get list of FPGA IPs available via the environment.
 * @return Vector of FPGA IPs as strings
 */
std::vector<std::string> get_fpga_ip_list() SYMBOL_VISIBLE;


/**
 * Get FPGA IP available via the environment.
 * @return FPGA IP as string
 */
std::string get_fpga_ip() SYMBOL_VISIBLE;

} // namespace hxcomm
