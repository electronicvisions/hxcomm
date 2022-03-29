#pragma once
#include "hate/visibility.h"
#include "hxcomm/vx/utmessage.h"
#include <vector>

namespace hxcomm::vx::detail {

struct QuiggeldyScheduleOutToInTransform
{
	using request_type = std::vector<UTMessageToFPGAVariant>;
	using response_type = std::vector<UTMessageFromFPGAVariant>;

	request_type operator()(response_type const& response, request_type const& snapshot)
	    SYMBOL_VISIBLE;
};

} // namespace hxcomm::vx::detail
