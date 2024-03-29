#pragma once
#include "hxcomm/common/utmessage.h"
#include <cereal/cereal.hpp>
#include <cereal/types/hate/bitset.h>

namespace cereal {

template <
    typename Archive,
    size_t HeaderAlignment,
    typename SubwordType,
    typename PhywordType,
    typename Dictionary,
    typename Instruction>
void CEREAL_SERIALIZE_FUNCTION_NAME(
    Archive& ar,
    hxcomm::UTMessage<HeaderAlignment, SubwordType, PhywordType, Dictionary, Instruction>& message)
{
	auto ut_message_payload = message.get_payload();
	ar(CEREAL_NVP(ut_message_payload));
	message.set_payload(ut_message_payload);
}

} // namespace cereal
