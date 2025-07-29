#include "hxcomm/vx/multi_zeromockconnection.h"

#include "hxcomm/common/connection_registry.tcc"
#include "hxcomm/common/multiconnection_impl.tcc"

namespace hxcomm {

template class MultiConnection<hxcomm::vx::ZeroMockConnection>;

template class ConnectionRegistry<hxcomm::vx::MultiZeroMockConnection>;

} // namespace hxcomm::vx