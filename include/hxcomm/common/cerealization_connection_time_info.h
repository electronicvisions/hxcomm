#pragma once

#include "hxcomm/common/connection_time_info.h"

#include <cereal/cereal.hpp>
#include <cereal/types/chrono.hpp>

namespace cereal {

template <typename Archive>
void CEREAL_SERIALIZE_FUNCTION_NAME(Archive& ar, hxcomm::ConnectionTimeInfo& cti)
{
	ar(cti.encode_duration, cti.decode_duration, cti.commit_duration, cti.execution_duration);
}

} // namespace cereal
