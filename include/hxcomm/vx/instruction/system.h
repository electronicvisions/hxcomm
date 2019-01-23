#pragma once
#include "hxcomm/common/payload.h"

/** Instructions for setting up an experiment. */
namespace hxcomm::vx::instruction::system {

/**
 * Instruction to set reset register.
 * True enables the reset.
 */
struct reset
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::bitset<reset, size> payload_type;
};

/** Instruction generating a halt response. */
struct halt
{
	constexpr static size_t size = 0;
	typedef hxcomm::instruction::detail::payload::bitset<reset, size> payload_type;
};

/** Dictionary of all system instructions. */
typedef hate::type_list<reset, halt> dictionary;

} // namespace hxcomm::vx::instruction::system
