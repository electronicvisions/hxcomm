#pragma once
#include "hxcomm/vx/instruction/event_from_fpga.h"
#include "hxcomm/vx/instruction/event_to_fpga.h"
#include "hxcomm/vx/instruction/from_fpga_system.h"
#include "hxcomm/vx/instruction/jtag_from_hicann.h"
#include "hxcomm/vx/instruction/omnibus_from_fpga.h"
#include "hxcomm/vx/instruction/omnibus_to_fpga.h"
#include "hxcomm/vx/instruction/system.h"
#include "hxcomm/vx/instruction/timing.h"
#include "hxcomm/vx/instruction/timing_from_fpga.h"
#include "hxcomm/vx/instruction/to_fpga_jtag.h"

namespace hxcomm::vx::instruction {

/** Dictionary containing all to_fpga instruction subsets. */
typedef hate::multi_concat_t<
    to_fpga_jtag::Dictionary,
    timing::Dictionary,
    system::Dictionary,
    omnibus_to_fpga::Dictionary,
    event_to_fpga::Dictionary>
    ToFPGADictionary;

/** Dictionary containing all from_fpga instruction subsets. */
typedef hate::multi_concat_t<
    jtag_from_hicann::Dictionary,
    omnibus_from_fpga::Dictionary,
    from_fpga_system::Dictionary,
    timing_from_fpga::Dictionary,
    event_from_fpga::Dictionary>
    FromFPGADictionary;

} // namespace hxcomm::vx::instruction
