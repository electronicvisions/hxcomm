#pragma once
#include "hxcomm/common/payload.h"

/** JTAG responses transported back to the host. */
namespace hxcomm::vx::instruction::jtag_from_hicann {

/** Response to a to_fpga_jtag::Data packet, if keep_response is true. */
struct Data
{
	constexpr static size_t size = 33;
	typedef hxcomm::instruction::detail::payload::Bitset<Data, size> Payload;
};

/** Dictionary of all jtag_from_hicann instructions. */
typedef hate::type_list<Data> Dictionary;

} // namespace hxcomm::vx::instruction::jtag_from_hicann
