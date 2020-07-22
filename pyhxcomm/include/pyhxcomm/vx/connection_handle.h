#pragma once

#include "pyhxcomm/common/managed_connection.h"

#include "hxcomm/vx/connection_variant.h"

namespace pyhxcomm::vx {

using ConnectionHandle = typename detail::add_handle<hxcomm::vx::ConnectionVariant>::type;

} // namespace pyhxcomm::vx
