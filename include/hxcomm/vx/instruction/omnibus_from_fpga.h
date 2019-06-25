#pragma once
#include <climits>

#include "hxcomm/common/payload.h"

/** Omnibus from FPGA responses transported back to the host. */
namespace hxcomm::vx::instruction::omnibus_from_fpga {

/** Response to a omnibus_to_fpga::Address packet, if is_read is true. */
struct Data
{
	constexpr static size_t size = sizeof(uint32_t) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Bitset<Data, size> Payload;
};

/** Dictionary of all omnibus from FPGA instructions. */
typedef hate::type_list<Data> Dictionary;

} // namespace hxcomm::vx::instruction::omnibus_from_fpga
