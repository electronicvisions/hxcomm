#pragma once
#include "hxcomm/common/payload.h"

/** JTAG responses transported back to the host. */
namespace hxcomm::vx::instruction::jtag_from_hicann {

/** Response to a to_fpga_jtag::data packet, if keep_response is true. */
struct data
{
	constexpr static size_t size = 33;
	typedef hxcomm::instruction::detail::payload::bitset<data, size> payload_type;
};

/** Dictionary of all jtag_from_hicann instructions. */
typedef hate::type_list<data> dictionary;

} // namespace hxcomm::vx::instruction::jtag_from_hicann
