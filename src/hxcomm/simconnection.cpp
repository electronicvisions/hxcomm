#include "hxcomm/simconnection.h"

namespace hxcomm {

SimConnection::SimConnection(ip_t ip, port_t port) : SimulatorClient(ip, port) {}

} // namespace hxcomm
