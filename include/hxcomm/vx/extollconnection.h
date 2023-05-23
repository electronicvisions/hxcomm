#pragma once

#include "hxcomm/common/extollconnection.h"
#include "hxcomm/vx/connection_parameter.h"

namespace hxcomm::vx {

using ExtollConnection = hxcomm::ExtollConnection<ConnectionParameter>;

} // namespace hxcomm::vx

namespace hxcomm {

extern template class SYMBOL_VISIBLE ExtollConnection<hxcomm::vx::ConnectionParameter>;
extern template class SYMBOL_VISIBLE ConnectionRegistry<hxcomm::vx::ExtollConnection>;

} // namespace hxcomm