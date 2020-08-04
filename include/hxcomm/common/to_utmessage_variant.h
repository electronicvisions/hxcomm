#pragma once
#include "hate/type_list.h"
#include <boost/variant.hpp>

namespace hxcomm {

template <
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
class UTMessage;


/**
 * Get UT message variant type corresponding to UT message parameters.
 * @tparam HeaderAlignment Alignment of header in bits
 * @tparam SubwordType Type of subword which's width corresponds to the messages alignment
 * @tparam PhywordType Type of PHY-word which's width corresponds to the message's minimal width
 * @tparam Dictionary Dictionary of instructions
 */
template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename Dictionary>
struct ToUTMessageVariant;


template <size_t HeaderAlignment, typename SubwordType, typename PhywordType, typename... Is>
struct ToUTMessageVariant<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...> >
{
	typedef boost::variant<
	    UTMessage<HeaderAlignment, SubwordType, PhywordType, hate::type_list<Is...>, Is>...>
	    type;
};

} // namespace hxcomm
