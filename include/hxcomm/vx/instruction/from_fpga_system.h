#pragma once
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include <climits>

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
	constexpr static size_t size = 45;
	typedef hxcomm::instruction::detail::payload::Bitset<TimeoutNotification, size> Payload;
};

/** Dictionary of all FPGA system response instructions. */
typedef hate::type_list<Loopback, HighspeedLinkNotification, TimeoutNotification> Dictionary;

} // namespace hxcomm::vx::instruction::from_fpga_system
