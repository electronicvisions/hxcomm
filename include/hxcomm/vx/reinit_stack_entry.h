#pragma once

#include "hxcomm/vx/quiggeldy_connection.h"

#include "hxcomm/common/reinit_stack_entry.h"
#include "hxcomm/vx/connection_variant.h"


namespace hxcomm::vx {

using ReinitStackEntry =
    hxcomm::ReinitStackEntry<hxcomm::vx::QuiggeldyConnection, hxcomm::vx::ConnectionVariant>;

} // namespace hxcomm::vx

namespace hxcomm {

extern template class SYMBOL_VISIBLE
    ReinitStackEntry<hxcomm::vx::QuiggeldyConnection, hxcomm::vx::ConnectionVariant>;

} // namespace hxcomm
