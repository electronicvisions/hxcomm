#include "hxcomm/vx/multi_simconnection.h"

#include "hxcomm/common/connection_registry.tcc"
#include "hxcomm/common/multiconnection_impl.tcc"

namespace hxcomm {

template class MultiConnection<hxcomm::vx::SimConnection>;

template class ConnectionRegistry<hxcomm::vx::MultiSimConnection>;

} // namespace hxcomm::vx