#include "hxcomm/vx/extollconnection.h"

#include "hxcomm/common/connection_registry.tcc"
#include "hxcomm/common/extollconnection_impl.tcc"

namespace hxcomm {

template class ExtollConnection<hxcomm::vx::ConnectionParameter>;

template class ConnectionRegistry<hxcomm::vx::ExtollConnection>;

} // namespace hxcomm
