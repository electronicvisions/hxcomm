#pragma once
#include "hate/visibility.h"
#include "hxcomm/common/simconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

using SimConnection = hxcomm::SimConnection<ConnectionParameter>;

} // namespace hxcomm::vx

namespace hxcomm {

extern template struct SYMBOL_VISIBLE ConnectionRegistry<hxcomm::vx::SimConnection>;

} // namespace hxcomm
