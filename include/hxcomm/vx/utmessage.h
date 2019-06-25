#pragma once
#include "hxcomm/common/utmessage.h"
#include "hxcomm/vx/instruction/instruction.h"

namespace hxcomm::vx {

constexpr size_t ut_message_to_fpga_header_alignment = 8;
typedef uint64_t ut_message_to_fpga_subword_type;
typedef uint64_t ut_message_to_fpga_phyword_type;

constexpr size_t ut_message_from_fpga_header_alignment = 8;
typedef uint64_t ut_message_from_fpga_subword_type;
typedef uint64_t ut_message_from_fpga_phyword_type;

template <typename I>
using UTMessageToFPGA = UTMessage<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    ut_message_to_fpga_phyword_type,
    instruction::ToFPGADictionary,
    I>;

template <typename I>
using UTMessageFromFPGA = UTMessage<
    ut_message_from_fpga_header_alignment,
    ut_message_from_fpga_subword_type,
    ut_message_from_fpga_phyword_type,
    instruction::FromFPGADictionary,
    I>;

using UTMessageToFPGAVariant = ToUTMessageVariant<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    ut_message_to_fpga_phyword_type,
    instruction::ToFPGADictionary>::type;

using UTMessageFromFPGAVariant = ToUTMessageVariant<
    ut_message_from_fpga_header_alignment,
    ut_message_from_fpga_subword_type,
    ut_message_from_fpga_phyword_type,
    instruction::FromFPGADictionary>::type;

} // namespace hxcomm::vx
