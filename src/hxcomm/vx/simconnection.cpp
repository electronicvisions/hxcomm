#include "hxcomm/vx/simconnection.h"

#include "hxcomm/common/connection_registry.tcc"
#include "hxcomm/common/simconnection_impl.tcc"

namespace hxcomm {

template class SimConnection<hxcomm::vx::ConnectionParameter>;

template class ConnectionRegistry<hxcomm::vx::SimConnection>;

} // namespace hxcomm
