#pragma once

#include "hxcomm/common/connection_full_stream_interface_variant.h"
#ifdef WITH_HXCOMM_HOSTARQ
#include "hxcomm/vx/arqconnection.h"
#endif
#ifdef WITH_HXCOMM_EXTOLL
#include "hxcomm/vx/extollconnection.h"
#endif
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/simconnection.h"
#include "hxcomm/vx/zeromockconnection.h"

#include <variant>

namespace hxcomm::vx {

using ConnectionVariant = std::variant<
#ifdef WITH_HXCOMM_HOSTARQ
    hxcomm::vx::ARQConnection,
#endif
#ifdef WITH_HXCOMM_EXTOLL
    hxcomm::vx::ExtollConnection,
#endif
    hxcomm::vx::SimConnection,
    hxcomm::vx::QuiggeldyConnection,
    hxcomm::vx::ZeroMockConnection>;

using ConnectionFullStreamInterfaceVariant =
    hxcomm::ConnectionFullStreamInterfaceVariant<ConnectionVariant>;

} // namespace hxcomm::vx
