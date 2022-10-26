#pragma once

#include "hate/visibility.h"
#include "hxcomm/common/arqconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

using ARQConnection = hxcomm::ARQConnection<ConnectionParameter>;

} // namespace hxcomm::vx

namespace hxcomm {

extern template struct SYMBOL_VISIBLE ConnectionRegistry<hxcomm::vx::ARQConnection>;

} // namespace hxcomm
