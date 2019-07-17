#include "hxcomm/vx/arqconnection.h"

namespace hxcomm::vx {

ARQConnection::ARQConnection() : hxcomm::ARQConnection<ConnectionParameter>() {}

ARQConnection::ARQConnection(ip_t ip) : hxcomm::ARQConnection<ConnectionParameter>(ip) {}

} // namespace hxcomm::vx
