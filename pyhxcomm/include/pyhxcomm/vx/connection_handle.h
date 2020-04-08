#pragma once

#include "pyhxcomm/common/connection_handle.h"

#include "hxcomm/vx/connection_parameter.h"

namespace pyhxcomm::vx {

using ConnectionHandle = pyhxcomm::ConnectionHandle<hxcomm::vx::ConnectionParameter>;

} // namespace pyhxcomm::vx
