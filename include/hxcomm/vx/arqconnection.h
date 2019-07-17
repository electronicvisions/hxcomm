#pragma once
#include "hxcomm/common/arqconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

class ARQConnection : public hxcomm::ARQConnection<ConnectionParameter>
{
public:
	ARQConnection();

	ARQConnection(ip_t ip);
};

} // namespace hxcomm::vx
