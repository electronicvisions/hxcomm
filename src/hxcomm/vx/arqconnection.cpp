#include "hxcomm/vx/arqconnection.h"

#include "hxcomm/common/arqconnection_impl.tcc"
#include "hxcomm/common/connection_registry.tcc"

namespace hxcomm {

template class ARQConnection<hxcomm::vx::ConnectionParameter>;

template class ConnectionRegistry<hxcomm::vx::ARQConnection>;

} // namespace hxcomm
