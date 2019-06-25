#pragma once
#include "hxcomm/common/payload.h"

/** Instructions for setting up an experiment. */
namespace hxcomm::vx::instruction::system {

/**
 * Instruction to set reset register.
 * True enables the reset.
 */
struct Reset
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::Bitset<Reset, size> Payload;
};

/** Instruction generating a Halt response. */
struct Halt
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::Bitset<Halt, size> Payload;
};

/** Dictionary of all system instructions. */
typedef hate::type_list<Reset, Halt> Dictionary;

} // namespace hxcomm::vx::instruction::system
