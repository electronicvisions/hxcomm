#pragma once
#include <stddef.h>

/** Constants for event instructions to and from the FPGA. */
namespace hxcomm::vx::instruction::event_constants {

constexpr size_t max_num_packed = 3;

constexpr size_t spike_size = 16;

} // namespace hxcomm::vx::instruction::event_constants
