#pragma once
#include "hate/visibility.h"
#ifdef WITH_EXTOLL
#include "rma2.h"
#endif
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


#ifdef WITH_EXTOLL
/**
 * Convert FPGA IP to Extoll Node ID
 * @return RMA2_Nodeid
 */
std::optional<RMA2_Nodeid> convert_ip_to_extollid(std::string) SYMBOL_VISIBLE;


/**
 * Convert list of FPGA IPs to list of Extoll Node IDs.
 * @return Vector of RMA2_Nodeid
 */
std::vector<RMA2_Nodeid> convert_ips_to_extollids(std::vector<std::string>) SYMBOL_VISIBLE;
#endif // WITH_EXTOLL

} // namespace hxcomm
