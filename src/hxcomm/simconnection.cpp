#include "hxcomm/vx/simconnection.h"

namespace hxcomm::vx {

SimConnection::SimConnection(ip_t ip, port_t port) :
    hxcomm::SimConnection<ConnectionParameter>(ip, port)
{}

} // namespace hxcomm::vx
