#pragma once
#include "hxcomm/common/simconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

class SimConnection : public hxcomm::SimConnection<ConnectionParameter>
{
public:
	SimConnection(ip_t ip, port_t port);
};

} // namespace hxcomm::vx
