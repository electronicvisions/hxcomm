#pragma once

#include "hxcomm/common/connection_full_stream_interface_variant.h"
#ifdef WITH_HXCOMM_HOSTARQ
#include "hxcomm/vx/multi_arqconnection.h"
#endif
#include "hxcomm/vx/multi_simconnection.h"
#include "hxcomm/vx/multi_zeromockconnection.h"
#include "hxcomm/vx/quiggeldy_connection.h"

#include <variant>

namespace hxcomm::vx {

using ConnectionVariant = std::variant<
#ifdef WITH_HXCOMM_HOSTARQ
    hxcomm::vx::MultiARQConnection,
#endif
    hxcomm::vx::MultiSimConnection,
    hxcomm::vx::MultiZeroMockConnection,
    hxcomm::vx::QuiggeldyConnection>;

} // namespace hxcomm::vx
