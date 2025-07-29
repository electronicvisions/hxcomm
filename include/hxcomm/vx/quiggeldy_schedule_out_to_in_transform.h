#pragma once
#include "hate/visibility.h"
#include "hxcomm/vx/utmessage.h"
#include <vector>

namespace hxcomm::vx::detail {

struct QuiggeldyScheduleOutToInTransform
{
	using request_type = std::vector<std::vector<UTMessageToFPGAVariant>>;
	using response_type = std::vector<std::vector<UTMessageFromFPGAVariant>>;

	request_type operator()(response_type const& responses, request_type const& snapshots)
	    SYMBOL_VISIBLE;
};

} // namespace hxcomm::vx::detail
