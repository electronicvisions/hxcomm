#pragma once
#include <climits>
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"

/** Instructions controlling the timing of execution of commands. */
namespace hxcomm::vx::instruction::timing {

/** Initialize timer to 0. */
struct Setup
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::Bitset<Setup, size> Payload;
};

/** Block further execution until incrementing timer reaches specified count. */
struct WaitUntil
{
	typedef uint32_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Number<WaitUntil, value_type> Payload;
};

/** Syncronize ASIC time with FPGA time. */
struct SystimeInit
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::Bitset<SystimeInit, size> Payload;
};

/** Dictionary of all timing instructions. */
typedef hate::type_list<Setup, WaitUntil, SystimeInit> Dictionary;

} // namespace hxcomm::vx::instruction::timing
