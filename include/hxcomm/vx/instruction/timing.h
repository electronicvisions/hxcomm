#pragma once
#include <climits>
#include "hxcomm/common/payload.h"

/** Instructions controlling the timing of execution of commands. */
namespace hxcomm::vx::instruction::timing {

/** Initialize timer to 0. */
struct setup
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::bitset<setup, size> payload_type;
};

/** Block further execution until incrementing timer reaches specified count. */
struct wait_until
{
	typedef uint32_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::number<wait_until, value_type> payload_type;
};

/** Dictionary of all timing instructions. */
typedef hate::type_list<setup, wait_until> dictionary;

} // namespace hxcomm::vx::instruction::timing
