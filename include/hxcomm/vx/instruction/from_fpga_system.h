#pragma once
#include <climits>
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"

/** FPGA system responses to the host. */
namespace hxcomm::vx::instruction::from_fpga_system {

/** Trace-marker response to a system::Loopback packet. */
struct Loopback
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::Bitset<Loopback, size> Payload;

	constexpr static Payload halt{0};
	constexpr static Payload tick{1};
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<Loopback> Dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
