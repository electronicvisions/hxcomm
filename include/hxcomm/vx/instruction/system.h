#pragma once
#include "hate/type_list.h"
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

/** Instruction generating a Loopback response. */
struct Loopback
{
	constexpr static size_t size = 1;
	typedef hxcomm::instruction::detail::payload::Bitset<Loopback, size> Payload;

	constexpr static Payload halt{0};
	constexpr static Payload tick{1};
};

/** Dictionary of all system instructions. */
typedef hate::type_list<Reset, Loopback> Dictionary;

} // namespace hxcomm::vx::instruction::system
