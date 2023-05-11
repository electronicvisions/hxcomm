#pragma once

#include "hxcomm/common/connection_full_stream_interface_variant.h"
#ifdef WITH_HOSTARQ
#include "hxcomm/vx/arqconnection.h"
#endif
#include "hxcomm/vx/extollconnection.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

#include <variant>

namespace hxcomm::vx {

using ConnectionVariant = std::variant<
#ifdef WITH_HOSTARQ
    hxcomm::vx::ARQConnection,
#endif
    hxcomm::vx::ExtollConnection,
    hxcomm::vx::SimConnection,
    hxcomm::vx::QuiggeldyConnection,
    hxcomm::vx::ZeroMockConnection>;

using ConnectionFullStreamInterfaceVariant =
    hxcomm::ConnectionFullStreamInterfaceVariant<ConnectionVariant>;

} // namespace hxcomm::vx
