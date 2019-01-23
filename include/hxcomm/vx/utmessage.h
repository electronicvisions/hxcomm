#pragma once
#include "hxcomm/common/utmessage.h"
#include "hxcomm/vx/instruction/instruction.h"

namespace hxcomm::vx {

constexpr size_t ut_message_to_fpga_header_alignment = 8;
typedef uint64_t ut_message_to_fpga_subword_type;

constexpr size_t ut_message_from_fpga_header_alignment = 8;
typedef uint64_t ut_message_from_fpga_subword_type;

template <typename I>
using ut_message_to_fpga = ut_message<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    instruction::to_fpga_dictionary,
    I>;

template <typename I>
using ut_message_from_fpga = ut_message<
    ut_message_from_fpga_header_alignment,
    ut_message_from_fpga_subword_type,
    instruction::from_fpga_dictionary,
    I>;

using ut_message_to_fpga_variant = to_ut_message_variant<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    instruction::to_fpga_dictionary>::type;

using ut_message_from_fpga_variant = to_ut_message_variant<
    ut_message_from_fpga_header_alignment,
    ut_message_from_fpga_subword_type,
    instruction::from_fpga_dictionary>::type;

} // namespace hxcomm::vx
