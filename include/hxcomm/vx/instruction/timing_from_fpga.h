#pragma once
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include <climits>

/** Timing response instructions from the FPGA. */
namespace hxcomm::vx::instruction::timing_from_fpga {

/** Full systime update. */
struct Systime
{
	constexpr static size_t size = 43;
	typedef hxcomm::instruction::detail::payload::Bitset<Systime, size> Payload;
};

/** Delta time update. */
struct Sysdelta
{
	constexpr static size_t size = 8;
	typedef hxcomm::instruction::detail::payload::Bitset<Sysdelta, size> Payload;
};

/** Dictionary of all timing response instructions from the FPGA. */
typedef hate::type_list<Systime, Sysdelta> Dictionary;

} // namespace hxcomm::vx::instruction::timing_from_fpga
