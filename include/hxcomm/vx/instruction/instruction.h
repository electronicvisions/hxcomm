#pragma once
#include "hxcomm/vx/instruction/from_fpga_system.h"
#include "hxcomm/vx/instruction/jtag_from_hicann.h"
#include "hxcomm/vx/instruction/system.h"
#include "hxcomm/vx/instruction/timing.h"
#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::vx::instruction {

/** Dictionary containing all to_fpga instruction subsets. */
typedef hate::multi_concat_t<to_fpga_jtag::dictionary, timing::dictionary, system::dictionary>
    to_fpga_dictionary;

/** Dictionary containing all from_fpga instruction subsets. */
typedef hate::multi_concat_t<jtag_from_hicann::dictionary, from_fpga_system::dictionary>
    from_fpga_dictionary;

} // namespace hxcomm::vx::instruction
