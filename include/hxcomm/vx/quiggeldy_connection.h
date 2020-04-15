#pragma once

#include "hxcomm/common/quiggeldy_connection.h"
#include "hxcomm/vx/quiggeldy_rcf.h"

namespace hxcomm::vx {

using QuiggeldyConnection =
    hxcomm::QuiggeldyConnection<hxcomm::vx::ConnectionParameter, hxcomm::vx::detail::rcf_client_type>;

} // namespace hxcomm::vx
