#pragma once

#include "hxcomm/common/connection_full_stream_interface_variant.h"
#ifdef WITH_HXCOMM_HOSTARQ
#include "hxcomm/vx/arqconnection.h"
#endif
#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

#include <variant>

namespace hxcomm::vx {

using SingleConnectionVariant = std::variant<
#ifdef WITH_HXCOMM_HOSTARQ
    hxcomm::vx::ARQConnection,
#endif
    hxcomm::vx::SimConnection,
    hxcomm::vx::ZeroMockConnection>;

using SingleConnectionFullStreamInterfaceVariant =
    hxcomm::ConnectionFullStreamInterfaceVariant<SingleConnectionVariant>;

} // namespace hxcomm::vx
