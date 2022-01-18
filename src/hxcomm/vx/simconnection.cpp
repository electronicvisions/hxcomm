#include "hxcomm/vx/simconnection.h"

#include "hxcomm/common/connection_registry.tcc"

namespace hxcomm {

template class ConnectionRegistry<hxcomm::vx::SimConnection>;

} // namespace hxcomm
