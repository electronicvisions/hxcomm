#pragma once
#include <climits>
#include "hxcomm/common/payload.h"

/** FPGA system responses to the host. */
namespace hxcomm::vx::instruction::from_fpga_system {

/** Trace-marker response to a system::halt packet. */
struct halt
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::bitset<halt, size> payload_type;
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<halt> dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
