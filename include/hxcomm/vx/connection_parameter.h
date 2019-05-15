#pragma once
#include "hxcomm/common/connection_parameter.h"
#include "hxcomm/vx/utmessage.h"

namespace hxcomm::vx {

typedef hxcomm::ConnectionParameter<
    ut_message_to_fpga_header_alignment,
    ut_message_to_fpga_subword_type,
    ut_message_to_fpga_phyword_type,
    instruction::to_fpga_dictionary,
    ut_message_from_fpga_header_alignment,
    ut_message_from_fpga_subword_type,
    ut_message_from_fpga_phyword_type,
    instruction::from_fpga_dictionary,
    instruction::from_fpga_system::halt>
    ConnectionParameter;

} // namespace hxcomm::vx
