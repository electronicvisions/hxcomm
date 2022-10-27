#pragma once
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include <climits>

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

/** Block further execution until specified set of communication channels is idle. */
struct Barrier
{
	constexpr static size_t size = 4;
	typedef hxcomm::instruction::detail::payload::Bitset<Barrier, size> Payload;
	constexpr static Payload omnibus{0b0001};
	constexpr static Payload jtag{0b0010};
	constexpr static Payload systime{0b0100};
	constexpr static Payload multi_fpga{0b1000};
};

/**
 * Block further execution until specified Omnibus address has value & mask == target.
 * Configuration is found in the FPGA Omnibus register file of the executor.
 */
struct PollingOmnibusBlock
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::Bitset<PollingOmnibusBlock, size> Payload;
};

/** Dictionary of all timing instructions. */
typedef hate::type_list<Setup, WaitUntil, SystimeInit, Barrier, PollingOmnibusBlock> Dictionary;

} // namespace hxcomm::vx::instruction::timing
