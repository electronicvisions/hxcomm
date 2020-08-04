#pragma once
#include "hate/type_list.h"
#include "hxcomm/common/payload.h"
#include <climits>

/** Omnibus from FPGA responses transported back to the host. */
namespace hxcomm::vx::instruction::omnibus_from_fpga {

/** Response to a omnibus_to_fpga::Address packet, if is_read is true. */
struct Data
{
	typedef uint32_t value_type;
	constexpr static size_t size = sizeof(value_type) * CHAR_BIT;
	typedef hxcomm::instruction::detail::payload::Number<Data, value_type> Payload;
};

/** Dictionary of all omnibus from FPGA instructions. */
typedef hate::type_list<Data> Dictionary;

} // namespace hxcomm::vx::instruction::omnibus_from_fpga
