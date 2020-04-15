#pragma once

#include "hxcomm/common/connection_full_stream_interface_variant.h"
#include "hxcomm/vx/arqconnection.h"
#include "hxcomm/vx/quiggeldy_connection.h"
#include "hxcomm/vx/simconnection.h"

#include <variant>

namespace hxcomm::vx {

using ConnectionVariant =
    std::variant<hxcomm::vx::ARQConnection, hxcomm::vx::SimConnection, hxcomm::vx::QuiggeldyConnection>;

using ConnectionFullStreamInterfaceVariant =
    hxcomm::ConnectionFullStreamInterfaceVariant<ConnectionVariant>;

} // namespace hxcomm::vx
