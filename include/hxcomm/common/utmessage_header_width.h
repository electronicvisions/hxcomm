#pragma once
#include "hate/math.h"
#include "hate/type_list.h"

namespace hxcomm {

/**
 * Get the header width corresponding to a given set of UT message parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename Dictionary>
struct UTMessageHeaderWidth
{
	static constexpr size_t value = hate::math::round_up_to_multiple(
	    1 /* comma width */ +
	        hate::math::num_bits(
	            hate::type_list_size<Dictionary>::value - 1 /* max. header index */),
	    HeaderAlignment);
};

} // namespace hxcomm
