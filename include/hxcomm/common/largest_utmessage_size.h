#pragma once
#include "hate/type_list.h"
#include <algorithm>

namespace hxcomm {

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
class UTMessage;


/**
 * Get the largest UT message size in bits for a given set of UT message parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam SubwordType Type of subword which's width corresponds to the messages alignment
 * @tparam PhywordType Type of PHY-word which's width corresponds to the message's minimal width
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary>
struct LargestUTMessageSize;


template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct LargestUTMessageSize<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	static constexpr size_t value =
	    std::max({UTMessage<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>::
	                  word_width...});
};

} // namespace hxcomm
