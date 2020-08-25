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

/** Highspeed-Link notifications. */
struct HighspeedLinkNotification
{
	constexpr static size_t size = 8;
	typedef hxcomm::instruction::detail::payload::Bitset<HighspeedLinkNotification, size> Payload;
};

/** Instruction timeout notification. */
struct TimeoutNotification
{
	typedef uint32_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Number<TimeoutNotification, value_type> Payload;
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<Loopback, HighspeedLinkNotification, TimeoutNotification> Dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
