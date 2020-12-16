#pragma once

#include "hxcomm/common/reinit_stack_entry.h"
#include "hxcomm/vx/connection_variant.h"
#include "hxcomm/vx/quiggeldy_connection.h"

namespace hxcomm::vx {

using ReinitStackEntry =
    hxcomm::ReinitStackEntry<hxcomm::vx::QuiggeldyConnection, hxcomm::vx::ConnectionVariant>;

} // namespace hxcomm::vx
