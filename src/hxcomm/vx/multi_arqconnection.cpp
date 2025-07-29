#include "hxcomm/vx/multi_arqconnection.h"

#include "hxcomm/common/connection_registry.tcc"
#include "hxcomm/common/multiconnection_impl.tcc"

namespace hxcomm {

template class MultiConnection<hxcomm::vx::ARQConnection>;

template class ConnectionRegistry<hxcomm::vx::MultiARQConnection>;

} // namespace hxcomm::vx