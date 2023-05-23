#include "hxcomm/vx/quiggeldy_connection.h"

#include "hxcomm/common/quiggeldy_connection_impl.tcc"

namespace hxcomm {

template class QuiggeldyConnection<
    hxcomm::vx::ConnectionParameter,
    hxcomm::vx::detail::rcf_client_type>;

} // namespace hxcomm
