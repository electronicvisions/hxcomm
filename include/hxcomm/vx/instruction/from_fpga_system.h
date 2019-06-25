#pragma once
#include <climits>
#include "hxcomm/common/payload.h"

/** FPGA system responses to the host. */
namespace hxcomm::vx::instruction::from_fpga_system {

/** Trace-marker response to a system::Halt packet. */
struct Halt
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::Bitset<Halt, size> Payload;
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<Halt> Dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
