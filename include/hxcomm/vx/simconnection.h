#pragma once
#include "hxcomm/common/simconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

struct SimConnection : public hxcomm::SimConnection<ConnectionParameter>
{
	using connection_t::connection_t;
};

} // namespace hxcomm::vx
