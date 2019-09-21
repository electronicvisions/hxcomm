#pragma once
#include "hxcomm/common/arqconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

struct ARQConnection : public hxcomm::ARQConnection<ConnectionParameter>
{
	using connection_t::connection_t;
};

} // namespace hxcomm::vx
