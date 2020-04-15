#pragma once

#include "hxcomm/common/quiggeldy_server.h"
#include "hxcomm/vx/quiggeldy_rcf.h"

namespace hxcomm::vx {

template <typename Connection>
using QuiggeldyServer = hxcomm::QuiggeldyServer<Connection>;

} // namespace hxcomm::vx
